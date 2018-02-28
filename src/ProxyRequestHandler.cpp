#include "logging/aixlog.hpp"
#include "ProxyRequestHandler.h"

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>

#include <Poco/Net/HTTPServerConnectionFactory.h>
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
#include <Poco/Net/HTTPServerSession.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Exception.h>

using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;

void ProxyRequestHandler::sendPlainRequest(HTTPServerRequest &req, std::string path, std::ostream& out, Poco::URI uri) {
  // send request
  LOG(INFO) << "Creating session to " << uri.getHost() << std::endl;
  HTTPClientSession session(uri.getHost(), uri.getPort());
  HTTPRequest proxy_req(req.getMethod(), path, HTTPMessage::HTTP_1_1);
  session.sendRequest(proxy_req);

  // get response
  HTTPResponse res;
  LOG(INFO) << res.getStatus() << " - " << res.getReason() << std::endl;
  // create istream for session response
  istream &is = session.receiveResponse(res);

  // Copy HTTP stream to app server response stream
  Poco::StreamCopier::copyStream(is, out);

}

void ProxyRequestHandler::serveRequest(HTTPServerRequest &req, std::ostream& out) {
  try
  {
  // prepare session
  Poco::URI uri(req.getURI());

  // prepare path
  string path(uri.getPathAndQuery());
  if (path.empty()) path = "/";

  sendPlainRequest(req, path, out, uri);

  LOG(INFO) << "Requesting url=" << uri.getHost() << std::endl
    << "port=" << uri.getPort() << endl
    << "path=" << path << endl;

  }
  catch( const SSLException& e )
  {
      LOG(ERROR) << e.what() << ": " << e.message() << std::endl;
  }
  catch (Poco::Exception &ex)
  {
  LOG(ERROR) << "Failed to get response from url=" << req.getURI() << std::endl
    << "method=" << req.getMethod() << std::endl
    << ex.what() << ": " << ex.message() << endl;
  }
}

void ProxyRequestHandler::handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp)
{

  LOG(INFO) << "Handle Request" << std::endl;

  resp.setStatus(HTTPResponse::HTTP_OK);

  std::ostream& out = resp.send();

  resp.setContentType("text/html");
  LOG(INFO) << "Plain request" << std::endl;
  serveRequest(req, out);

  //out << resp.getStatus() << " - " << resp.getReason() << endl;
  LOG(INFO) << resp.getStatus() << " - " << resp.getReason() << std::endl;
}

ProxyRequestHandler::ProxyRequestHandler() {

}
ProxyRequestHandler::~ProxyRequestHandler() {

}
