#include "logging/aixlog.hpp"
#include "HTTPServerConnection.h"

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
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Exception.h>

using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;

class MyRequestHandler : public HTTPRequestHandler
{
public:

  void connect(HTTPServerRequest &req, HTTPServerResponse &resp, ostream& out) {
    // CONNECT
    LOG(INFO) << "HTTP CONNECT" << endl
      << "Host=" << req.getHost() << endl;

  }

  void sendPlainRequest(HTTPServerRequest &req, string path, ostream& out, Poco::URI uri) {
    // send request
    LOG(INFO) << "Creating session to " << uri.getHost() << endl;
    HTTPClientSession session(uri.getHost(), uri.getPort());
    HTTPRequest proxy_req(req.getMethod(), path, HTTPMessage::HTTP_1_1);
    session.sendRequest(proxy_req);

    // get response
    HTTPResponse res;
    LOG(INFO) << res.getStatus() << " - " << res.getReason() << endl;
    // create istream for session response
    istream &is = session.receiveResponse(res);

    // Copy HTTP stream to app server response stream
    Poco::StreamCopier::copyStream(is, out);

  }

  void serveRequest(HTTPServerRequest &req, ostream& out) {
    try
    {
    // prepare session
    Poco::URI uri(req.getURI());

    // prepare path
    string path(uri.getPathAndQuery());
    if (path.empty()) path = "/";

    sendPlainRequest(req, path, out, uri);

    LOG(INFO) << "Requesting url=" << uri.getHost() << endl
      << "port=" << uri.getPort() << endl
      << "path=" << path << endl;

    }
    catch( const SSLException& e )
    {
        LOG(ERROR) << e.what() << ": " << e.message() << endl;
    }
    catch (Poco::Exception &ex)
    {
    LOG(ERROR) << "Failed to get response from url=" << req.getURI() << endl
      << "method=" << req.getMethod() << endl
      << ex.what() << ": " << ex.message() << endl;
    }
  }

  virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp)
  {

    resp.setStatus(HTTPResponse::HTTP_OK);

    ostream& out = resp.send();

    if (req.getMethod() == "CONNECT") {
      //resp.setContentType("NO_CONTENT_TYPE");
      connect(req, resp, out);
      // TODO
    } else {
      resp.setContentType("text/html");
      LOG(INFO) << "Plain request" << endl;
      serveRequest(req, out);
    }

    //out << resp.getStatus() << " - " << resp.getReason() << endl;
    LOG(INFO) << resp.getStatus() << " - " << resp.getReason() << endl;
  }

private:
  static int count;
};

int MyRequestHandler::count = 0;

class MyRequestHandlerFactory : public HTTPRequestHandlerFactory
{
public:
  virtual HTTPRequestHandler* createRequestHandler(const HTTPServerRequest &)
  {
    LOG(INFO) << "Create request handler" << endl;
    return new MyRequestHandler;
  }

};

class tcpHandlerFactory: public TCPServerConnectionFactory
	/// This implementation of a TCPServerConnectionFactory
	/// is used by HTTPServer to create HTTPServerConnection objects.
{
public:
	tcpHandlerFactory(HTTPServerParams::Ptr pParams, HTTPRequestHandlerFactory::Ptr pFactory) :
	_pParams(pParams),
	_pFactory(pFactory) {

  };
		/// Creates the HTTPServerConnectionFactory.

	~tcpHandlerFactory() {

  };
		/// Destroys the HTTPServerConnectionFactory.

    virtual TCPServerConnection* createConnection(const StreamSocket& socket)
  {
    LOG(INFO) << "create connection" << endl;
  	return new HTTPServerConnection(socket, _pParams, _pFactory);
  }
		/// Creates an instance of HTTPServerConnection
		/// using the given StreamSocket.

private:
	HTTPServerParams::Ptr          _pParams;
	HTTPRequestHandlerFactory::Ptr _pFactory;
};

class MyServerApp : public ServerApplication
{
protected:
  int main(const vector<string> &)
  {
    //HTTPServer s(new MyRequestHandlerFactory, ServerSocket(8080), new HTTPServerParams);
    TCPServer s(new tcpHandlerFactory(new HTTPServerParams, new MyRequestHandlerFactory), ServerSocket(8080), new TCPServerParams);

    s.start();
    LOG(INFO) << endl << "Server started" << endl;

    waitForTerminationRequest();  // wait for CTRL-C or kill

    LOG(INFO) << endl << "Shutting down..." << endl;
    s.stop();

    return Application::EXIT_OK;
  }
};

int main(int argc, char *argv[])
{
  auto sink_cout = make_shared<AixLog::SinkCout>(AixLog::Severity::trace, AixLog::Type::normal);
  auto sink_file = make_shared<AixLog::SinkFile>(AixLog::Severity::trace, AixLog::Type::all, "/var/log/erss/proxy.log");
  AixLog::Log::init({sink_cout, sink_file});

  MyServerApp app;
  return app.run(argc, argv);
}
