//
// ProxyServerConnection.cpp
//
// Library: Net
// Package: HTTPServer
// Module:  ProxyServerConnection
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//

#include "logging/aixlog.hpp"

#include "ProxyServerConnection.h"
#include <Poco/Net/HTTPServerSession.h>
#include <Poco/Net/HTTPServerRequestImpl.h>
#include <Poco/Net/HTTPServerResponseImpl.h>
#include <Poco/Net/NetException.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Timestamp.h>
#include <Poco/Delegate.h>
#include <memory>
#include <iostream>
#include <Poco/Thread.h>

namespace Poco {
namespace Net {

class DestinationRelay:public Poco::Runnable {
	public:
		DestinationRelay(HTTPServerSession& sess, StreamSocket& dest, string h): session(sess), destinationServer(dest), host(h) {}

		virtual void run() {

			// 128 KB for destination (assumption is destination will pass more data than client)
			unsigned char destinationBuffer[131072];

			bool isOpen = true;

			Poco::Timespan readPollTimeOut(10,500);
			Poco::Timespan writePollTimeOut(1,0);

			LOG(DEBUG) << destinationServer.getBlocking() << "-" << destinationServer.getKeepAlive() << "-" << destinationServer.getNoDelay()
			<< "-" << destinationServer.getReceiveBufferSize() << "-" << destinationServer.getReceiveTimeout().seconds() << "-"
			<< destinationServer.getSendBufferSize() << "-" << destinationServer.getSendTimeout().seconds() << endl;

			while(isOpen){
					int nDestinationBytes = -1;

					try {
						if (destinationServer.poll(readPollTimeOut, Socket::SELECT_READ) == true) {
							LOG(DEBUG) << "Destination server has more requests! host=" << host << endl  << flush;
							nDestinationBytes = destinationServer.receiveBytes(destinationBuffer, sizeof(destinationBuffer));
						}
						if (nDestinationBytes > 0 && session.socket().poll(writePollTimeOut, Socket::SELECT_WRITE) == true) {
							LOG(DEBUG) << "Number of bytes received from destination=" << nDestinationBytes << "host=" << host << endl << flush;
							LOG(DEBUG) << "Sending bytes to client server... host=" << host << std::endl;
							session.socket().sendBytes(destinationBuffer, nDestinationBytes);
							LOG(DEBUG) << "Sent bytes to client server. host=" << host << std::endl;
						}
					}
					catch (Poco::Exception& exc) {
							//Handle your network errors.
							LOG(ERROR) << "Network error=" << exc.displayText() << "host=" << host << endl;
							isOpen = false;
					}

						if (nDestinationBytes == 0){
								LOG(DEBUG) << "Client or destination closes connection! host=" << host << endl << flush;
								isOpen = false;
						}

			}
		}

		private:
			HTTPServerSession& session;
			StreamSocket destinationServer;
			std::string host;
	};

ProxyServerConnection::ProxyServerConnection(const StreamSocket& socket, HTTPServerParams::Ptr pParams, ProxyRequestHandlerFactory::Ptr pFactory):
	TCPServerConnection(socket),
	_pParams(pParams),
	_pFactory(pFactory),
	_stopped(false)
{
	poco_check_ptr (pFactory);

	_pFactory->serverStopped += Poco::delegate(this, &ProxyServerConnection::onServerStopped);
}


ProxyServerConnection::~ProxyServerConnection()
{
	try
	{
		_pFactory->serverStopped -= Poco::delegate(this, &ProxyServerConnection::onServerStopped);
	}
	catch (...)
	{
		poco_unexpected();
	}
}

void ProxyServerConnection::run()
{
	std::string server = _pParams->getSoftwareVersion();
	HTTPServerSession session(socket(), _pParams);
	int count = 0;
	// Each thread has an HTTPServerSession obj and hence is either transmitting HTTP or HTTPS (post-connect) data
	bool connect = false;
	//StreamSocket destinationServer = StreamSocket();
	while (!_stopped && session.hasMoreRequests())
	{
		LOG(DEBUG) << "Has more" << std::endl;
		try
		{
			LOG(DEBUG) << "Grabbing lock." << std::endl;
			Poco::FastMutex::ScopedLock lock(_mutex);
			if (!_stopped)
			{

				count++;
				LOG(DEBUG) << "Request count=" <<  count << "from host=" << session.clientAddress().toString() << std::endl;

				//LOG(DEBUG) << "Creating response obj..." << std::endl;
				HTTPServerResponseImpl response(session);

				// MARK: - Past this point we are under the assumption that this is an uncencrypted HTTP Request
				//LOG(DEBUG) << "Creating request obj..." << std::endl;
				HTTPServerRequestImpl request(response, session, _pParams);

				LOG(DEBUG) << "Sucessfully parsed request." << std::endl;

				Poco::Timestamp now;
				response.setDate(now);
				response.setVersion(request.getVersion());

				response.setKeepAlive(_pParams->getKeepAlive() && request.getKeepAlive() && session.canKeepAlive());
				if (!server.empty())
					response.set("Server", server);
				try
				{
					std::unique_ptr<ProxyRequestHandler> pHandler(_pFactory->createRequestHandler(request));
					if (pHandler.get())
					{
					  if (request.expectContinue() && response.getStatus() == HTTPResponse::HTTP_OK)
							response.sendContinue();

						// This is an HTTP request so set httpData = true
						if (request.getMethod() != "CONNECT") {
							pHandler->handleRequest(request, response);
						} else {
							LOG(DEBUG) << "Potential CONNECT request for this session." << std::endl;
							connect = true;

							StreamSocket destinationServer = StreamSocket();
							string host(request.getHost());
							SocketAddress addr = SocketAddress(host);
							destinationServer.connect(addr);

							unsigned char okMessage[] = "200 OK";
							session.socket().sendBytes(okMessage, 7);

							destinationServer.setBlocking(false);
							session.socket().setBlocking(false);
							LOG(DEBUG) << "Handling request as HTTPS data. host=" << host << std::endl;

							Poco::Thread destThread;

							Poco::Net::DestinationRelay dRelay(session, destinationServer, host);
							destThread.start(dRelay);
							destThread.join();
							relayClientData(session, destinationServer, host);
							continue;
						}

						LOG(DEBUG) << "responseStatus=" << response.getStatus() << std::endl;

						LOG(DEBUG) << "pParams keepAlive=" << _pParams->getKeepAlive() << " request keepAlive=" << request.getKeepAlive()
						<< " response keepAlive="<< response.getKeepAlive() << " session keepAlive=" << session.canKeepAlive() << std::endl;
						session.setKeepAlive(_pParams->getKeepAlive() && response.getKeepAlive() && session.canKeepAlive());

					}
					else sendErrorResponse(session, HTTPResponse::HTTP_NOT_IMPLEMENTED);
				}
				catch (Poco::Exception&)
				{
					//LOG(ERROR) << "EXCEPTION: requestHandling" << std::endl;

					if (!response.sent())
					{
						try
						{
							sendErrorResponse(session, HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
						}
						catch (...)
						{
						}
					}

					throw;
				}
			}
		}
		catch (NoMessageException&)
		{
			LOG(ERROR) << "EXCEPTION: Parse Data w/ NoMessage" << std::endl;
			break;
		}
		catch (MessageException&)
		{
			LOG(ERROR) << "EXCEPTION: Parse Data w/ Message" << std::endl;
			sendErrorResponse(session, HTTPResponse::HTTP_BAD_REQUEST);
		}
		catch (Poco::Exception&)
		{
			LOG(ERROR) << "EXCEPTION: Parse Data" << std::endl;
			if (session.networkException())
			{
				session.networkException()->rethrow();
			}
			else throw;
		}
	}
}

void ProxyServerConnection::relayClientData(HTTPServerSession& session, StreamSocket& destinationServer, std::string host) {

	// 64 KB for client (assumption is destination will pass more data than client)
	unsigned char clientBuffer[65336];

	bool isOpen = true;

	Poco::Timespan readPollTimeOut(10,500);
	Poco::Timespan writePollTimeOut(1,0);

	LOG(DEBUG) << session.socket().getBlocking() << "-" << session.socket().getKeepAlive() << "-" << session.socket().getNoDelay()
	<< "-" << session.socket().getReceiveBufferSize() << "-" << session.socket().getReceiveTimeout().seconds() << "-"
	<< session.socket().getSendBufferSize() << "-" << session.socket().getSendTimeout().seconds() << endl;

	while(isOpen){
			int nClientBytes = -1;

			try {
				if (session.socket().poll(readPollTimeOut, Socket::SELECT_READ) == true) {
					LOG(DEBUG) << "Session has more requests! host=" << host << endl  << flush;
					nClientBytes = session.socket().receiveBytes(clientBuffer, sizeof(clientBuffer));
				}

				if (nClientBytes > 0 && destinationServer.poll(writePollTimeOut, Socket::SELECT_WRITE) == true) {
					LOG(DEBUG) << "Number of bytes received from client=" << nClientBytes << " host=" << host << endl << flush;
					LOG(DEBUG) << "Sending bytes to destination server.. host=" << host << std::endl;
					destinationServer.sendBytes(clientBuffer, nClientBytes);
					LOG(DEBUG) << "Sent bytes to destination server. host=" << host << std::endl;
				}

			}
			catch (Poco::Exception& exc) {
					//Handle your network errors.
					LOG(ERROR) << "Network error=" << exc.displayText() << "host=" << host << endl;
					isOpen = false;
			}

				if (nClientBytes==0){
						LOG(DEBUG) << "Client or destination closes connection! host=" << host << endl << flush;
						isOpen = false;
				}

	}
}


void ProxyServerConnection::sendErrorResponse(HTTPServerSession& session, HTTPResponse::HTTPStatus status)
{
	LOG(ERROR) << "Send error response="
	<< status << std::endl;

	HTTPServerResponseImpl response(session);
	response.setVersion(HTTPMessage::HTTP_1_1);
	response.setStatusAndReason(status);
	response.setKeepAlive(false);
	response.send();
	session.setKeepAlive(false);
}


void ProxyServerConnection::onServerStopped(const bool& abortCurrent)
{
	_stopped = true;
	if (abortCurrent)
	{
		try
		{
			// Note: On Windows, select() will not return if one of its socket is being
			// shut down. Therefore we have to call close(), which works better.
			// On other platforms, we do the more graceful thing.
#if defined(_WIN32)
			socket().close();
#else
			socket().shutdown();
#endif
		}
		catch (...)
		{
		}
	}
	else
	{
		Poco::FastMutex::ScopedLock lock(_mutex);

		try
		{
#if defined(_WIN32)
			socket().close();
#else
			socket().shutdown();
#endif
		}
		catch (...)
		{
		}
	}
}


} } // namespace Poco::Net
