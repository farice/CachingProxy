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
//#include <Poco/Net/HTTPRequestHandler.h>
//#include "ProxyRequestHandler.h"
//#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/NetException.h>
#include <Poco/NumberFormatter.h>
#include <Poco/Timestamp.h>
#include <Poco/Delegate.h>
#include <memory>
#include <iostream>

namespace Poco {
namespace Net {


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
						if (request.getExpectContinue() && response.getStatus() == HTTPResponse::HTTP_OK)
							response.sendContinue();

						// This is an HTTP request so set httpData = true
						if (request.getMethod() != "CONNECT") {
							pHandler->handleRequest(request, response);
						} else {
							LOG(DEBUG) << "Potential CONNECT request for this session." << std::endl;
							connect = true;
							relayData(session, request.getHost());
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

void ProxyServerConnection::relayData(HTTPServerSession& session, std::string host) {

	// 10 KB for each buffer
	// This may create unrealistic memory overhead
	// TODO - Consider
	unsigned char clientBuffer[10000];
	unsigned char destinationBuffer[10000];

	bool isOpen = true;
	// 1s timeout when polling
	Poco::Timespan readTimeOut(0,500);
	Poco::Timespan writeTimeOut(1,0);

	StreamSocket destinationServer = StreamSocket();
	SocketAddress addr = SocketAddress(host);
	destinationServer.connect(addr);

	LOG(DEBUG) << session.socket().getBlocking() << "-" << session.socket().getKeepAlive() << "-" << session.socket().getNoDelay()
	<< "-" << session.socket().getReceiveBufferSize() << "-" << session.socket().getReceiveTimeout().seconds() << "-"
	<< session.socket().getSendBufferSize() << "-" << session.socket().getSendTimeout().seconds() << endl;

	LOG(DEBUG) << destinationServer.getBlocking() << "-" << destinationServer.getKeepAlive() << "-" << destinationServer.getNoDelay()
	<< "-" << destinationServer.getReceiveBufferSize() << "-" << destinationServer.getReceiveTimeout().seconds() << "-"
	<< destinationServer.getSendBufferSize() << "-" << destinationServer.getSendTimeout().seconds() << endl;

	unsigned char okMessage[] = "200 OK";
	session.socket().sendBytes(okMessage, 7);

	session.socket().setBlocking(false);
	destinationServer.setBlocking(false);
	//session.setKeepAlive(true);

	LOG(DEBUG) << "Handling request as HTTPS data. host=" << host << std::endl;

	// TODO - Poll each address (client and destination) in separate threads so we can use poll to signal and wake efficiently
	// For now, we just poll repeatedly and the overhead is seen clearly (compare HTTP vs. HTTPS load time)
	while(isOpen){
			int nClientBytes = -1;
			int nDestinationBytes = -1;

			try {
				if (session.socket().poll(readTimeOut, Socket::SELECT_READ) == true) {
					LOG(DEBUG) << "Session has more requests! host=" << host << endl  << flush;
					nClientBytes = session.socket().receiveBytes(clientBuffer, sizeof(clientBuffer));
				}

				if (nClientBytes > 0 && destinationServer.poll(writeTimeOut, Socket::SELECT_WRITE) == true) {
					LOG(DEBUG) << "Number of bytes received from client=" << nClientBytes << " host=" << host << endl << flush;
					LOG(DEBUG) << "Sending bytes to destination server.. host=" << host << std::endl;
					destinationServer.sendBytes(clientBuffer, nClientBytes);
					LOG(DEBUG) << "Sent bytes to destination server. host=" << host << std::endl;
				}

				if (destinationServer.poll(readTimeOut, Socket::SELECT_READ) == true) {
					LOG(DEBUG) << "Destination server has more requests! host=" << host << endl  << flush;
					nDestinationBytes = destinationServer.receiveBytes(destinationBuffer, sizeof(destinationBuffer));
				}
				if (nDestinationBytes > 0 && session.socket().poll(writeTimeOut, Socket::SELECT_WRITE) == true) {
					LOG(DEBUG) << "Number of bytes received from destination=" << nDestinationBytes << "host=" << host << endl << flush;
					LOG(DEBUG) << "Sending bytes to client server... host=" << host << std::endl;
					session.socket().sendBytes(destinationBuffer, nDestinationBytes);
					LOG(DEBUG) << "Sent bytes to client server. host=" << host << std::endl;
					//Poco::StreamCopier::copyStream(destinationBuffer, out);
				}
			}
			catch (Poco::Exception& exc) {
					//Handle your network errors.
					LOG(ERROR) << "Network error=" << exc.displayText() << "host=" << host << endl;
					isOpen = false;
			}

				if (nClientBytes==0 || nDestinationBytes == 0){
						LOG(DEBUG) << "Client or destination closes connection! host=" << host << endl << flush;
						isOpen = false;
				}

//				else{

//					}

	}

	session.setKeepAlive(false);
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
