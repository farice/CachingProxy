#include "logging/aixlog.hpp"
#include "ProxyServerConnection.h"

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>

#include<Poco/Net/HTTPServerConnectionFactory.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Exception.h>

using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;

class ProxyHandlerFactory: public TCPServerConnectionFactory
	/// This implementation of a TCPServerConnectionFactory
	/// is used by HTTPServer to create HTTPServerConnection objects.
{
public:
	ProxyHandlerFactory(HTTPServerParams::Ptr pParams, ProxyRequestHandlerFactory::Ptr pFactory) :
	_pParams(pParams),
	_pFactory(pFactory) {

    poco_check_ptr (pFactory);
    LOG(DEBUG) << "keepAlive=" << pParams->getKeepAlive();

  };
		/// Creates the HTTPServerConnectionFactory.

	~ProxyHandlerFactory() {

  };
		/// Destroys the HTTPServerConnectionFactory.

    virtual TCPServerConnection* createConnection(const StreamSocket& socket)
  {
    LOG(DEBUG) << "create connection" << endl;
  	return new ProxyServerConnection(socket, _pParams, _pFactory);
  }
		/// Creates an instance of HTTPServerConnection
		/// using the given StreamSocket.

private:
	HTTPServerParams::Ptr          _pParams;
	ProxyRequestHandlerFactory::Ptr _pFactory;
};

class ProxyServerApp : public ServerApplication
{
protected:
  int main(const vector<string> &)
  {
    //HTTPServer s(new ProxyRequestHandlerFactory, ServerSocket(8080), new HTTPServerParams);
    TCPServer s(new ProxyHandlerFactory(new HTTPServerParams, new ProxyRequestHandlerFactory), ServerSocket(8080), new HTTPServerParams);

    s.start();
    LOG(DEBUG) << endl << "Server started" << endl;

    waitForTerminationRequest();  // wait for CTRL-C or kill

    LOG(DEBUG) << endl << "Shutting down..." << endl;
    s.stop();

    return Application::EXIT_OK;
  }
};

int main(int argc, char *argv[])
{

  // Initialize logging
  auto sink_cout = make_shared<AixLog::SinkCout>(AixLog::Severity::trace, AixLog::Type::normal);
  auto sink_file = make_shared<AixLog::SinkFile>(AixLog::Severity::trace, AixLog::Type::all, "/var/log/erss/proxy.log");
  AixLog::Log::init({sink_cout, sink_file});

  // run proxy app (Poco ServerApplication)
  ProxyServerApp app;
  return app.run(argc, argv);
}
