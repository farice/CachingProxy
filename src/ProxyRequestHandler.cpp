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

void ProxyRequestHandler::handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp)
{

  std::ostream& out = resp.send();

  try
  {
  // prepare session
  Poco::URI uri(req.getURI());

  // prepare path
  string path(uri.getPathAndQuery());
  if (path.empty()) path = "/";

  // send request
  LOG(DEBUG) << "Creating session to " << uri.getHost() << std::endl;
  HTTPClientSession session(uri.getHost(), uri.getPort());
  HTTPRequest proxy_req(req.getMethod(), path, HTTPMessage::HTTP_1_1);

  // POST only
  if(proxy_req.getMethod() == "POST") {
    LOG(DEBUG) << "POST request to=" << uri.getHost() << std::endl;
    proxy_req.setContentType("application/x-www-form-urlencoded");
    proxy_req.setContentLength(req.getContentLength());
    Poco::Net::NameValueCollection cookies;

    LOG(DEBUG) << "Post content length=" << proxy_req.getContentLength() << std::endl;
    req.getCookies(cookies);
    proxy_req.setCookies(cookies);
    //LOG(DEBUG) << "csrf_token=" << req.get("csrf_token");

    string body("csrfmiddlewaretoken=qtHiu8G3zxkIMWG3tSe2paIvrpNYxSxmTCB1AaMZoCsmF8t8mLsN3rGq2Po5Hf8l&username=farice&password=farice23&next=");
    std::ostream& opost = session.sendRequest(proxy_req);
    session.sendRequest(proxy_req) << body;
    std::istream &ipost = req.stream();

    LOG(DEBUG) << "Writing body to POST stream=" << body << endl;

    //opost << body;
    Poco::StreamCopier::copyStream(ipost, opost);

    //proxy_req.write(std::cout);

  } else { // GET / other (per requirements this is the only other HTTP request we are concerned with)

    proxy_req.setContentType("text/html");
    session.sendRequest(proxy_req);
  }
  // endif POST


  // get response
  HTTPResponse proxy_resp;
  // create istream for session response
  istream &is = session.receiveResponse(proxy_resp);
  LOG(DEBUG) << "Proxy resp: " << proxy_resp.getStatus() << " - " << proxy_resp.getReason() << std::endl;
  // Copy HTTP stream to app server response stream
  Poco::StreamCopier::copyStream(is, out);

  LOG(DEBUG) << "Requesting url=" << uri.getHost() << std::endl
    << "port=" << uri.getPort() << endl
    << "path=" << path << endl;

    resp.setStatus(proxy_resp.getStatus());
    resp.setContentType(proxy_resp.getContentType());
  }
  catch (Poco::Exception &ex)
  {
  LOG(ERROR) << "Failed to get response from url=" << req.getURI() << std::endl
    << "method=" << req.getMethod() << std::endl
    << ex.what() << ": " << ex.message() << endl;

    resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
  }


  LOG(DEBUG) << resp.getStatus() << " - " << resp.getReason() << std::endl;
}

ProxyRequestHandler::ProxyRequestHandler() {

}
ProxyRequestHandler::~ProxyRequestHandler() {

}
