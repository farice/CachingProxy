
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
  std::vector<std::pair<std::string,std::string> > cacheControlHeaders = getCacheControlHeaders(resp);
  for (int i = 0; i < cacheControlHeaders.size(); i++){
    if ((cacheControlHeaders[i].second == "no-store") ||
	(cacheControlHeaders[i].second == "private")){
      return false;
    }
  }
  return true;
}


bool ProxyRequestHandler::hasNoCacheDirective(HTTPResponse& resp){
  std::vector<std::pair<std::string,std::string> > cacheControlHeaders = getCacheControlHeaders(resp);
  for (int i = 0; i < cacheControlHeaders.size(); i++){
    if (cacheControlHeaders[i].second == "no-cache"){
      return true;
    }
  }
  return false;
}

// string NO_ETAG indicates no strong validator found
std::string ProxyRequestHandler::getEtag(HTTPResponse& resp){
  NameValueCollection::ConstIterator it = resp.begin();
  while (it != resp.end()){
    if (it->first == "ETag"){
      return it->second;
    }
    ++it;
  }
  return "NO_ETAG";
}


// -1 indicates no max-age directive found
double ProxyRequestHandler::getMaxAge(HTTPResponse& resp){
  std::vector<std::pair<std::string,std::string> > cacheControlHeaders = getCacheControlHeaders(resp);
  for (int i = 0; i < cacheControlHeaders.size(); i++){
    std::string current = cacheControlHeaders[i].second;
    if (current.find("max-age=") != string::npos){
      return atof((current.substr(current.find("=") + 1, string::npos)).c_str());
    }
  }
  return -1;
}


/*
std::string ProxyRequestHandler::getDate(HTTPResponse& resp){
  // can get data directly from request with poco library function
  // maybe just store it as to_string? doesn't really matter

}
*/

// string NO_EXPIRE indicates no expiration header was found
std::string ProxyRequestHandler::getExpires(HTTPResponse& resp){
  NameValueCollection::ConstIterator it = resp.begin();
  while (it != resp.end()){
    if (it->first == "Expires"){
      return it->second;
    }
    ++it;
  }
  return "NO_EXPIRE";
}



std::string ProxyRequestHandler::getLastModified(HTTPResponse& resp){
  NameValueCollection::ConstIterator it = resp.begin();
  while (it != resp.end()){
    if (it->first == "Last-Modified"){
      return it->second;
    }
    ++it;
  }
  return "NO_LAST_MODIFY";
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

void ProxyRequestHandler::remoteGet(Poco::URI& uri, std::string path, HTTPServerRequest& req, HTTPServerResponse& resp) {
  HTTPClientSession session(uri.getHost(), uri.getPort());
  HTTPRequest proxy_req(req.getMethod(), path, HTTPMessage::HTTP_1_1);
  string key = this->staticCache.makeKey(uri);

  // send request normally
  proxy_req.setContentType("text/html");
  session.sendRequest(proxy_req);

  // get response
  HTTPResponse proxy_resp;
  // create istream for session response
  istream &is = session.receiveResponse(proxy_resp);

  // copies cookies, headers, etc
  ProxyServerCache::copyResponseObj(proxy_resp, resp);

  ostringstream oss;
  oss << is.rdbuf();  // extract stream contents for caching
  if ((req.getMethod() == "GET") && (proxy_resp.getStatus() == 200)){
    // add if 200-OK resp to GET request

if (isCacheableResp(proxy_resp)){
// filter out 'no-store' and 'private' responses
LOG(TRACE) << "The response is cachable " << endl;

// now parse to find the Etag, lastModified, and other header fields

std::string etag = getEtag(proxy_resp);

std::string lastModified = getLastModified(proxy_resp);

std::string expires = getExpires(proxy_resp);

double maxAge = getMaxAge(proxy_resp);

    // determine expiration

    // testing basic function for now
this->staticCache.add(key, CacheResponse(proxy_resp, oss.str()));
}
  }

  LOG(TRACE) << "Proxy resp: " << proxy_resp.getStatus() << " - " << proxy_resp.getReason()
  << std::endl;

  LOG(TRACE) << "Requesting url=" << uri.getHost() << std::endl
  << "port=" << uri.getPort() << endl
  << "path=" << path << endl;

  std::ostream& out = resp.send();

  // Copy HTTP stream to app server response stream
  out << oss.str();
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

      ProxyServerCache::copyResponseObj(proxy_resp, resp);

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
        bool  validItem = false;

        if (!checkResponse.isNull()){

          if (!validItem) {
            // TODO: - update cacheitem depending on result of If-None-Match request
            updateCacheItem(uri, path, req, resp, checkResponse);
          }

          // writes to resp
          LOG(DEBUG) << "Copying back cached headers" << endl;
          (*checkResponse).getResponse(resp);
          std::ostream& out = resp.send();
          out << (*checkResponse).getResponseData().str();

          LOG(DEBUG) << "Response is cached, responding with cached data" << endl;
          return;
        }
        else{

          LOG(DEBUG) << "The request is not in the cache" << endl;
          remoteGet(uri, path, req, resp);

        }
      } else {
          remoteGet(uri, path, req, resp);
      }

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

void ProxyRequestHandler::updateCacheItem(Poco::URI uri, std::string path, HTTPServerRequest& req, HTTPServerResponse &resp, Poco::SharedPtr<CacheResponse> item) {

  HTTPResponse cacheResponseObj;
  (*item).getResponse(cacheResponseObj);
  string etag = "";
  if(cacheResponseObj.has("Etag")) {
    etag = cacheResponseObj.get("Etag");
    LOG(DEBUG) << "cached response has etag=" << etag << std::endl;
    HTTPClientSession session(uri.getHost(), uri.getPort());
    HTTPRequest ping_req(HTTPRequest::HTTP_GET, path, HTTPMessage::HTTP_1_1);
    ping_req.add("If-None-Match", etag);
    // send request normally
    session.sendRequest(ping_req);

    // get response
    HTTPResponse ping_resp;
    session.receiveResponse(ping_resp);

    LOG(DEBUG) << "status code from ping=" << ping_resp.getStatus() << std::endl;

    if (ping_resp.getStatus() == 304) {
      LOG(DEBUG) << "potentially stale data remains valid" << std::endl;

      // TODO - Update max age?

      return;
    }
  }

  remoteGet(uri, path, req, resp);


}


ProxyRequestHandler::ProxyRequestHandler(){
  //LOG(TRACE) << "------ Created Proxy Request Handler ------" << endl;
}


ProxyRequestHandler::~ProxyRequestHandler() {

}

// Cache is a static member shared by all ProxyRequestHandler objects
ProxyServerCache ProxyRequestHandler::staticCache;
