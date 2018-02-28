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

/*
void ProxyServerConnection::run() {
	cout << "New connection from: " << socket().peerAddress().host().toString() <<  endl << flush;
        bool isOpen = true;
        Poco::Timespan timeOut(10,0);
        unsigned char incomingBuffer[1000];
        while(isOpen){
            if (socket().poll(timeOut,Poco::Net::Socket::SELECT_READ) == false){
                cout << "TIMEOUT!" << endl << flush;
            }
            else{
                cout << "RX EVENT!!! ---> "   << flush;
                int nBytes = -1;

                try {
                    nBytes = socket().receiveBytes(incomingBuffer, sizeof(incomingBuffer));
                }
                catch (Poco::Exception& exc) {
                    //Handle your network errors.
                    cerr << "Network error: " << exc.displayText() << endl;
                    isOpen = false;
                }


                if (nBytes==0){
                    cout << "Client closes connection!" << endl << flush;
                    isOpen = false;
                }
                else{
                    cout << "Receiving nBytes: " << nBytes << endl << flush;
										cout << "Bytes" << incomingBuffer << endl << flush;
                }
            }
        }
        cout << "Connection finished!" << endl << flush;
    }
*/

void ProxyServerConnection::run()
{
	std::string server = _pParams->getSoftwareVersion();
	HTTPServerSession session(socket(), _pParams);
	int count = 0;
	// Each thread has an HTTPServerSession obj and hence is either transmitting HTTP or HTTPS (post-connect) data
	bool connect = false;
	StreamSocket destinationServer = StreamSocket();
	while (!_stopped && session.hasMoreRequests())
	{
		LOG(INFO) << "Has more" << std::endl;
		try
		{
			LOG(INFO) << "Grabbing lock." << std::endl;
			Poco::FastMutex::ScopedLock lock(_mutex);
			if (!_stopped)
			{

				count++;
				LOG(INFO) << "Request count=" <<  count << "from host=" << session.clientAddress().toString() << std::endl;

				//LOG(INFO) << "Creating response obj..." << std::endl;
				HTTPServerResponseImpl response(session);

				if (connect) {
					unsigned char clientBuffer[10000];
					unsigned char destinationBuffer[10000];

					bool isOpen = true;
					// 10s timeout
	        Poco::Timespan timeOut(60,0);

					while(isOpen){

					if (session.hasMoreRequests() == false && destinationServer.poll(timeOut, Socket::SELECT_READ) == false){
							LOG(INFO) << "Timeout" << endl << flush;
					}
					else{
							LOG(INFO) << "Status: SELECT_READ" << endl  << flush;
							int nClientBytes = -1;
							int nDestinationBytes = -1;

							try {
								if (session.hasMoreRequests() == true) {
									nClientBytes = session.socket().receiveBytes(clientBuffer, sizeof(clientBuffer));
								}
								if (destinationServer.poll(timeOut, Socket::SELECT_READ) == true) {
									nDestinationBytes = destinationServer.receiveBytes(destinationBuffer, sizeof(destinationBuffer));
								}
								if (nClientBytes > 0 && destinationServer.poll(timeOut, Socket::SELECT_WRITE) == true) {
									LOG(INFO) << "Number of bytes received from client=" << nClientBytes << endl << flush;
									LOG(INFO) << "Client Bytes=" << clientBuffer << endl << flush;
									LOG(INFO) << "Send bytes to destination server" << std::endl;
									destinationServer.sendBytes(clientBuffer, nClientBytes);
								}

								if (nDestinationBytes > 0 && session.socket().poll(timeOut, Socket::SELECT_WRITE) == true) {
									LOG(INFO) << "Number of bytes received from destination=" << nDestinationBytes << endl << flush;
									LOG(INFO) << "Destination Bytes=" << destinationBuffer << endl << flush;
									LOG(INFO) << "Send bytes to client server" << std::endl;
									session.socket().sendBytes(destinationBuffer, nDestinationBytes);
								}

									//LOG(INFO) << "data=" << incomingBuffer << endl << flush;
							}
							catch (Poco::Exception& exc) {
									//Handle your network errors.
									LOG(ERROR) << "Network error=" << exc.displayText() << endl;
									isOpen = false;
							}
			//				if (nClientBytes==0 && nDestinationBytes==0){
			//						LOG(INFO) << "Client and server close connection!" << endl << flush;
			//						isOpen = false;
			//				}
			//				else{

		//					}
					}

					}
					// CONNECT Connection terminated
					//session.setKeepAlive(false);
					continue;
				}

				// MARK: - Past this point we are under the assumption that this is an uncencrypted HTTP Request
				//LOG(INFO) << "Creating request obj..." << std::endl;
				HTTPServerRequestImpl request(response, session, _pParams);

				LOG(INFO) << "Sucessfully parsed request." << std::endl;

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
						pHandler->handleTCPRequest(request, response, true);
						LOG(INFO) << "responseStatus=" << response.getStatus() << std::endl;

						if (request.getMethod() == "CONNECT" && response.getStatus() == HTTPResponse::HTTP_ACCEPTED) {
							//LOG(INFO) << "Establishing connection with host=" << request.getHost().toString() << std::endl;
							SocketAddress addr = SocketAddress(request.getHost());
							destinationServer.connect(addr);
							connect = true;
							LOG(INFO) << "Succesful CONNECT request for this session." << std::endl;
						}

						LOG(INFO) << "pParams keepAlive=" << _pParams->getKeepAlive() << " request keepAlive=" << request.getKeepAlive()
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
