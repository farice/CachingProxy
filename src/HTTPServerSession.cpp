//
// HTTPServerSession.cpp
//
// Library: Net
// Package: HTTPServer
// Module:  HTTPServerSession
//
// Copyright (c) 2005-2006, Applied Informatics Software Engineering GmbH.
// and Contributors.
//
// SPDX-License-Identifier:	BSL-1.0
//


#include "HTTPServerSession.h"
#include <iostream>

namespace Poco {
namespace Net {


HTTPServerSession::HTTPServerSession(const StreamSocket& socket, HTTPServerParams::Ptr pParams):
	HTTPSession(socket, pParams->getKeepAlive()),
	_firstRequest(true),
	_keepAliveTimeout(pParams->getKeepAliveTimeout()),
	_maxKeepAliveRequests(pParams->getMaxKeepAliveRequests())
{

	std::cout << "Receive timeout=" << pParams->getTimeout().days() << "d-" << pParams->getTimeout().hours() << "h-"
	<< pParams->getTimeout().minutes() << "m-" << pParams->getTimeout().seconds() << "s" << std::endl;
	setTimeout(pParams->getTimeout());
	this->socket().setReceiveTimeout(pParams->getTimeout());
}


HTTPServerSession::~HTTPServerSession()
{
}


bool HTTPServerSession::hasMoreRequests()
{
	std::cout << "POLL SOCKET" << std::endl;

	if (!socket().impl()->initialized()) return false;

	std::cout << "Socket initialized." << std::endl;

	if (_firstRequest)
	{
		std::cout << "BP 2" << std::endl;
		_firstRequest = false;
		--_maxKeepAliveRequests;
		return socket().poll(getTimeout(), Socket::SELECT_READ);
	}
	else if (_maxKeepAliveRequests != 0 && getKeepAlive())
	{
		std::cout << "BP 3" << std::endl;
		if (_maxKeepAliveRequests > 0)

			--_maxKeepAliveRequests;
		return buffered() > 0 || socket().poll(_keepAliveTimeout, Socket::SELECT_READ);
	}
	else {
		std::cout << "Return false for hasMoreRequests()" << " maxKeepalive=" << _maxKeepAliveRequests << " getKeepAlive=" << getKeepAlive() << std::endl;
		return false;
	}
}


SocketAddress HTTPServerSession::clientAddress()
{
	return socket().peerAddress();
}


SocketAddress HTTPServerSession::serverAddress()
{
	return socket().address();
}


} } // namespace Poco::Net
