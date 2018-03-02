#include "logging/aixlog.hpp"
#include "ProxyRequestHandler.h"

#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>
#include <sstream>

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


// Key will be a combination of fields within the URI
string makeKey(Poco::URI& uri){
  string key(uri.getHost());
  key.append(to_string(uri.getPort()));
  key.append(uri.getPathAndQuery());
  LOG(DEBUG) << "Constructed key = " << key << endl;
  return key;
}


string istreamToStr(istream& is){
  string str;
  char buff[4096];
  while (is.read(buff, sizeof(buff))){
    str.append(buff, sizeof(buff));
  }
  str.append(buff, is.gcount());
  return str;
}

// Note we are only concerned here with the responses to get requests
pair<int,int> getCacheControl(HTTPServerResponse& resp){
  int freshness;
  if (resp.has("Cache-Control")) {

    const string controlVal = resp.get("Cache-control");

    LOG(DEBUG) << "Cache-Control=" << resp.get("Cache-control") << endl;

    if (controlVal.find("max-age=") != string::npos){
      freshness = stoi(controlVal.substr(controlVal.find("=") + 1, string::npos));
      LOG(DEBUG) << "freshness from max-age = " << freshness << endl;
      pair<int,int> ret(0, freshness);
      return ret;
    }
    else if (controlVal.find("s-maxage=") != string::npos){
      // overrides the max age of the resource in the response
      freshness = stoi(controlVal.substr(controlVal.find("=") + 1, string::npos));
      // need way to indicate that this is override, maybe return a std::pair with one element
      // being an indicator
      pair<int, int> ret(1, freshness);
      return ret;
    }
    else if (controlVal.find("max-stale") != string::npos){
      // can use an expired response, or a response that has expired by no more than x seconds
      int max_stale_time = stoi(controlVal.substr(controlVal.find("=") + 1, string::npos));
      pair<int,int> ret(2, max_stale_time);
      // note, the time is an optional argument, may need to lookout for thsi
      return ret;
    }
    else if (controlVal.find("min-fresh") != string::npos){
      int min_fresh_time = stoi(controlVal.substr(controlVal.find("=") + 1, string::npos));
      pair<int,int> ret(3, min_fresh_time);
      return ret;
    }
    else if (controlVal.find("no-store") != string::npos){
      // code here for no-store
      //freshness = -1; // -1 to indicate no-store directive
      pair<int,int> ret(-1,-1);
      return ret;
    }
    else if (controlVal.find("private") != string::npos){
      pair<int,int> ret(-1,-1);
      return ret;
    }
    else if (controlVal.find("no-cache") != string::npos){
      // item must be re-validated before it is served
      //freshness = -2; // indicate need for re-validation
      pair<int,int> ret(-2,-2);
      return ret;
    }
    else if (controlVal.find("must-revalidate") != string::npos){
      // item must be re-validated IF it is stale
      pair<int,int> ret(-3,-3);
      return ret;
    }
    else if (controlVal.find("only-if-cached") != string::npos){
      // only use the cached response if it exists
      pair<int,int> ret(-4,-4);
      return ret;
    }
    else{
      // no meaningful cache-control directive
      pair<int, int> ret(-10,-10);
      return ret;
    }
    // We won't worry about the 'no-transform' directive because we won't be modifying entries
    // We won't worry about the 'stale-while-revalidate' and 'stale-if-error' directives as they are
    // experimental, non-standard features not supported by most browsers
  }
  pair<int, int> ret(-10,-10);
  return ret;
}
  /*
    Expiration time is determined by more than the cache-control header
    - may be easier to just return a struct with all the information the cache needs to
    - consume


  */

void ProxyRequestHandler::handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp)
{

  // Only using HTTP requests (no danger of HTTPS)

  std::ostream& out = resp.send();

  // its the response that has cache control, not request
  /*
  LOG(DEBUG) << "Plain request" << std::endl;
  if (req.has("Cache-Control")) {
    LOG(DEBUG) << "Cache-Control=" << req.get("Cache-control") << endl;
  }
  */
  
  try
  {
  // prepare session
  Poco::URI uri(req.getURI());

  // prepare path
  string path(uri.getPathAndQuery());
  if (path.empty()) path = "/";

  // Once the request is constructed and before the request is sent
  // check the cache for that request


  string key = makeKey(uri); // construct key from uri

  /*
  this->requestCache.add(test, ExpRespStream("testResponseValue", 10000));
  //this->requestCache.add(uri, ExpRespStream("testResponseValue", 10000));

  poco_assert(this->requestCache.size() == 1);
  */


  // If it's in the cache, use the stored istream or data to fulfill the response


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

    // Here is where the check should be done (right before the request)
    session.sendRequest(proxy_req);
  }
  // endif POST

  // get response
  HTTPResponse proxy_resp;
  // create istream for session response
  istream &is = session.receiveResponse(proxy_resp);

  if (proxy_resp.has("Cache-Control")) {
    LOG(DEBUG) << "Cache-Control=" << proxy_resp.get("Cache-control") << endl;
  }
  //string respString(istreambuf_iterator<char>(is), {}); // works but is slow
  //string respString = istreamToStr(is); // works but unsure of errors

  this->requestCache->add("wombo","womboing");
  this->requestCache->add("wombology","it's first grade");
  poco_assert(this->requestCache->size() == 2);

  Poco::SharedPtr<string> element = this->requestCache->get("wombo");
  poco_assert(*element == "womboing");

  /*
  string responseVal;
  ostringstream oss;
  oss << is.rdbuf();
  responseVal = oss.str();


  out << responseVal.data();
  */

  //LOG(DEBUG) << "The responseVal string  = " << responseVal << endl;



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

ProxyRequestHandler::ProxyRequestHandler():requestCache(nullptr) {
  LOG(DEBUG) << "------ Created Proxy Request Handler ------" << endl;

}

ProxyRequestHandler::ProxyRequestHandler(Poco::LRUCache<std::string, std::string>* cache):requestCache(cache){
  LOG(DEBUG) << "------ Created Proxy Request Handler with cache ------" << endl;

}

ProxyRequestHandler::~ProxyRequestHandler() {

}
