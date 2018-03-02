
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
#include<Poco/Timestamp.h>
#include<Poco/DateTimeFormatter.h>

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
  LOG(TRACE) << "Cache-control headers" << endl;
  for (int i = 0; i < headers.size(); i++){
    cout << "Name=" << headers[i].first << ", value=" << headers[i].second << endl;
  }
  return headers;
}


// Not cacheable if has directives: no-store, private
// no-cache: may implement this directive, may not, functionality is met but just
// forwarding the response to the client in this case

// must-revalidate makes you re-validate a cached response if it is stale
// no-cache makes you re-validate a cached response everytime
bool ProxyRequestHandler::isCacheableResp(HTTPResponse& resp){
  std::vector<std::pair<std::string,std::string> > cacheControlHeaders = getCacheControlHeaders(resp\
												);
  for (int i = 0; i < cacheControlHeaders.size(); i++){
    if ((cacheControlHeaders[i].second == "no-store") ||
	(cacheControlHeaders[i].second == "private")){
      return false;
    }
  }
  return true;
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

  Poco::Timestamp now;
  string fmt = "%w %b %f %H:%M:%S %Y";
  string timestamp_str = Poco::DateTimeFormatter::format(now, fmt);
  LOG(INFO) << req.get("unique_id") << ": \"" << req.getMethod() << " " << req.getHost() << " " << req.getVersion()
  << "\" from " << req.get("ip_addr") << " @ " << timestamp_str << std::endl;

  try
  {
    // prepare uri
    Poco::URI uri(req.getURI());

    // prepare path
    string path(uri.getPathAndQuery());
    if (path.empty()) path = "/";

    string key = this->staticCache.makeKey(uri); // construct cache key from uri

    // log send request
    LOG(TRACE) << "Creating proxy session to " << uri.getHost() << std::endl;

    // POST only
    if(req.getMethod() == "POST") {
      // prepare session
      HTTPClientSession session(uri.getHost(), uri.getPort());
      HTTPRequest proxy_req(req.getMethod(), path, HTTPMessage::HTTP_1_1);

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

      htmlBody.write(opost);
      /*
      LOG(TRACE) << "POST body=";
      htmlBody.write(LOG(TRACE));
      LOG(TRACE) << endl;
      */

      // get response
      HTTPResponse proxy_resp;
      // create istream for session response
      istream &is = session.receiveResponse(proxy_resp);

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
      Poco::StreamCopier::copyStream(is, out);

    }
    else {
      /* GET or other request method */

      if (req.getMethod() == "GET"){

        // check if the request has been cached
        Poco::SharedPtr<CacheResponse> checkResponse = this->staticCache.get(key);
        // TODO: - validation logic
        // add CacheResponseItem call that returns whether item needs to be revalidated to validItem
        validItem = false;

        if (!validItem) {
          // TODO: - return updated/same cacheitem depending on result of validate method (in this class)
          //
        }

        if (!checkResponse.isNull()){
          std::ostream& out = resp.send();
          out << (*checkResponse).getResponseData().str();
          LOG(DEBUG) << "Response is cached, responding with cached data" << endl;
          return;
        }
        else{
          LOG(DEBUG) << "The request is not in the cache" << endl;
        }
      }

      HTTPClientSession session(uri.getHost(), uri.getPort());
      HTTPRequest proxy_req(req.getMethod(), path, HTTPMessage::HTTP_1_1);

      // send request normally
      proxy_req.setContentType("text/html");
      session.sendRequest(proxy_req);

      // get response
      HTTPResponse proxy_resp;
      // create istream for session response
      istream &is = session.receiveResponse(proxy_resp);

      ostringstream oss;
      oss << is.rdbuf();  // extract stream contents for caching

      if ((req.getMethod() == "GET") && (proxy_resp.getStatus() == 200)){
        // add if 200-OK resp to GET request

	if (isCacheableResp(proxy_resp)){
	  // filter out 'no-store' and 'private' responses
	  LOG(TRACE) << "The response is cachable " << endl;

        // determine expiration

        // testing basic function for now
	  this->staticCache.add(key, CacheResponse(oss.str(), 10, false));
	}
      }

      LOG(TRACE) << "Proxy resp: " << proxy_resp.getStatus() << " - " << proxy_resp.getReason()
      << std::endl;

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
      out << oss.str();
    }
  }
  catch (Poco::Exception &ex){

    LOG(DEBUG) << "Failed to get response from url=" << req.getURI() << std::endl
    << "method=" << req.getMethod() << std::endl
    << ex.what() << ": " << ex.message() << endl;

    resp.setStatus(HTTPResponse::HTTP_BAD_REQUEST);
  }


  LOG(TRACE) << resp.getStatus() << " - " << resp.getReason() << std::endl;
}


ProxyRequestHandler::ProxyRequestHandler(){
  //LOG(TRACE) << "------ Created Proxy Request Handler ------" << endl;
}


ProxyRequestHandler::~ProxyRequestHandler() {

}

// Cache is a static member shared by all ProxyRequestHandler objects
ProxyServerCache ProxyRequestHandler::staticCache;
