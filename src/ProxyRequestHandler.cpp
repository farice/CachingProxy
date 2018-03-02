#include "logging/aixlog.hpp"
#include "ProxyRequestHandler.h"

#include <iostream>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>
#include <sstream>
#include <utility>

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
#include <Poco/Net/HTMLForm.h>
#include <Poco/Net/HTTPCookie.h>


using namespace Poco::Net;
using namespace Poco::Util;
using namespace std;


// Note we are only concerned here with the responses to get requests
pair<int,int> getCacheControl(HTTPServerResponse& resp){ // this function is trash

  int freshness;
  if (resp.has("Cache-Control")) {

    const string controlVal = resp.get("Cache-control");

    LOG(TRACE) << "Cache-Control=" << resp.get("Cache-control") << endl;

    if (controlVal.find("max-age=") != string::npos){
      freshness = stoi(controlVal.substr(controlVal.find("=") + 1, string::npos));
      LOG(TRACE) << "freshness from max-age = " << freshness << endl;
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

std::vector<std::pair<std::string,std::string> > ProxyRequestHandler::getCacheControlHeaders(HTTPResponse& resp){
  std::string name;
  std::string value;
  std::vector<std::pair<std::string,std::string> > headers;
  NameValueCollection::ConstIterator it = resp.begin();
  while (it != resp.end()){ // just cache-control part for now
    name = it->first;
    if (name == "Cache-Control"){
      value = it->second;
      headers.push_back(std::make_pair(name, value));
    }
    ++it;
  }
  for (int i = 0; i < headers.size(); i++){
    cout << "Name=" << headers[i].first << ", value=" << headers[i].second << endl;
  }
  return headers;
}

std::vector<std::pair<std::string,std::string> > ProxyRequestHandler::getHeaders(HTTPResponse& resp){
  std::string name;
  std::string value;
  std::vector<std::pair<std::string,std::string> > headers;
  NameValueCollection::ConstIterator it = resp.begin();
  while (it != resp.end()){
    name = it->first;
    value = it->second;
    headers.push_back(std::make_pair(name, value));
    ++it;
  }
  for (int i = 0; i < headers.size(); i++){
    cout << "Name=" << headers[i].first << ", value=" << headers[i].second << endl;
  }
  return headers;
}


void ProxyRequestHandler::handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp)
{
  /************************************************/
  // Only using HTTP requests (no danger of HTTPS) //
  /************************************************/
  request_id++;
  LOG(TRACE) << "HTTP request" << std::endl;

  try
    {
      // prepare session
      Poco::URI uri(req.getURI());

      // prepare path
      string path(uri.getPathAndQuery());
      if (path.empty()) path = "/";


  // Create the session after checking the original response method
  // We don't wanna make a session if we don't have to

  // send request
  LOG(TRACE) << "Creating proxy session to " << uri.getHost() << std::endl;
  HTTPClientSession session(uri.getHost(), uri.getPort());
  HTTPRequest proxy_req(req.getMethod(), path, HTTPMessage::HTTP_1_1);

  string key = this->staticCache.makeKey(uri); // construct key from uri

  // POST only
  if(proxy_req.getMethod() == "POST") {
    LOG(TRACE) << "POST request to=" << uri.getHost() << std::endl;
    proxy_req.setContentType("application/x-www-form-urlencoded");
    proxy_req.setKeepAlive(true);
    proxy_req.setContentLength(req.getContentLength());


    LOG(TRACE) << "Post content length=" << proxy_req.getContentLength() << std::endl;

    // Copy over cookies from client's request to proxy request
    Poco::Net::NameValueCollection postCookies;
    req.getCookies(postCookies);
    proxy_req.setCookies(postCookies);

    std::ostream& opost = session.sendRequest(proxy_req);

    HTMLForm htmlBody(req, req.stream());
    //std::istream &ipost = req.stream();
    //Poco::StreamCopier::copyStream(ipost, opost);
    htmlBody.write(opost);
    LOG(TRACE) << "POST body=";
    htmlBody.write(LOG(TRACE));
    LOG(TRACE) << endl;

  } else { // GET / other (per requirements this is the only other HTTP request we are concerned with)

    if (proxy_req.getMethod() == "GET"){

      Poco::SharedPtr<CacheResponse> checkResponse = this->staticCache.get(key);

      if (!checkResponse.isNull()){ // if the response is cached
	std::ostream& out = resp.send();
	out << (*checkResponse).getResponseData().str();
	LOG(DEBUG) << "{{{{++++ Responded with cached data brother ++++}}}}"
		   << endl;

	return;
      }
      else{
	LOG(DEBUG) << "The request is not in the cache" << endl;
      }
    }

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
    LOG(TRACE) << "Cache-Control=" << proxy_resp.get("Cache-control") << endl;
  }

  ostringstream oss;
  oss << is.rdbuf();
  if ((req.getMethod() == "GET") && (proxy_resp.getStatus() == 200)){ // add of 200-OK resp to GET

    this->staticCache.add(key, CacheResponse(oss.str(), 10, false)); // testing basic function for now
  }

  LOG(TRACE) << "Proxy resp: " << proxy_resp.getStatus() << " - " << proxy_resp.getReason() << std::endl;

  LOG(TRACE) << "Requesting url=" << uri.getHost() << std::endl
    << "port=" << uri.getPort() << endl
    << "path=" << path << endl;

  resp.setStatus(proxy_resp.getStatus());
  resp.setContentType(proxy_resp.getContentType());

  // Copy over cookies from proxy's response to client response
  std::vector<HTTPCookie> respCookies;
  proxy_resp.getCookies(respCookies);

  for (int i=0 ; i < respCookies.size(); i++) {
    HTTPCookie cookie(respCookies[i]);
    resp.addCookie(cookie);
  }

  std::ostream& out = resp.send();
  // Copy HTTP stream to app server response stream
  //Poco::StreamCopier::copyStream(is, out);
  out << oss.str();

  }
  catch (Poco::Exception &ex)
  {
  LOG(DEBUG) << "Failed to get response from url=" << req.getURI() << std::endl
    << "method=" << req.getMethod() << std::endl
    << ex.what() << ": " << ex.message() << endl;

      resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
    }


  LOG(TRACE) << resp.getStatus() << " - " << resp.getReason() << std::endl;
}

ProxyRequestHandler::ProxyRequestHandler():requestCache(nullptr) {
  //LOG(TRACE) << "------ Created Proxy Request Handler ------" << endl;

}

///*
ProxyRequestHandler::ProxyRequestHandler(ProxyServerCache* cache):requestCache(cache){
  //LOG(TRACE) << "------ Created Proxy Request Handler with cache ------" << endl;

}
//*/

ProxyRequestHandler::~ProxyRequestHandler() {

}

ProxyServerCache ProxyRequestHandler::staticCache;
int ProxyRequestHandler::request_id = 0;
