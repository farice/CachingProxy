#include "ProxyServerCache.h"
#include <Poco/DateTimeParser.h>

using namespace Poco;
using namespace std;
using Poco::DateTimeFormat;

// constructor
ProxyServerCache::ProxyServerCache(){
  //LOG(DEBUG) << "Yo, the cache is created with size = " << 1024 << std::endl;
}

// constructor (overloaded)
/*
ProxyServerCache::ProxyServerCache(size_t new_size):size(new_size){
  LOG(DEBUG) << "Yo, the cache is created with size = " << this->size << std::endl;
}
*/

// destructor
ProxyServerCache::~ProxyServerCache(){
  //LOG(DEBUG) << "Yo, the cache has been destroyed... " << std::endl;
}

std::string ProxyServerCache::makeKey(Poco::URI& uri){
  std::string key(uri.getHost());
  key.append(std::to_string(uri.getPort()));
  key.append(uri.getPathAndQuery());
  LOG(DEBUG) << "Constructed key = " << key << std::endl;
  return key;
}


void ProxyServerCache::copyResponseObj(const Poco::Net::HTTPResponse &fromResp, Poco::Net::HTTPResponse &toResp) {
  // Copy over headers
  Poco::Net::NameValueCollection::ConstIterator i = fromResp.begin();
  while(i!=fromResp.end()){
      //LOG(DEBUG) << "Copying over header=" <<  i->first << std::endl;
      toResp.add(i->first, i->second);
      //LOG(DEBUG) << "Copying over value=" <<  toResp.get(i->first) << std::endl;
      ++i;
  }

  // Copy over cookies from proxy's response to client response
  std::vector<Poco::Net::HTTPCookie> respCookies;
  fromResp.getCookies(respCookies);

  for (int i=0 ; i < respCookies.size(); i++) {
    Poco::Net::HTTPCookie cookie(respCookies[i]);
    toResp.addCookie(cookie);
  }

  toResp.setStatus(fromResp.getStatus());
  toResp.setContentType(fromResp.getContentType());
}

void ProxyServerCache::copyResponseObj(const Poco::Net::HTTPResponse &fromResp, Poco::Net::HTTPServerResponse &toResp) {
  // Copy over headers
  Poco::Net::NameValueCollection::ConstIterator i = fromResp.begin();
  while(i!=fromResp.end()){
      //LOG(DEBUG) << "Copying over header=" <<  i->first << std::endl;
      toResp.add(i->first, i->second);
      //LOG(DEBUG) << "Copying over value=" <<  toResp.get(i->first) << std::endl;
      ++i;
  }

  // Copy over cookies from proxy's response to client response
  std::vector<Poco::Net::HTTPCookie> respCookies;
  fromResp.getCookies(respCookies);

  for (int i=0 ; i < respCookies.size(); i++) {
    Poco::Net::HTTPCookie cookie(respCookies[i]);
    toResp.addCookie(cookie);
  }

  toResp.setStatus(fromResp.getStatus());
  toResp.setContentType(fromResp.getContentType());
}

/*
Expiration time is determined by more than the cache-control header
- may be easier to just return a struct with all the information the cache needs to
- consume
*/

std::map<std::string, std::string> ProxyServerCache::getCacheControlHeaders(const Poco::Net::HTTPResponse& resp){
  /*
  if (resp.has("cacheControlHeaders")){
    return resp.get("cacheControlHeaders");
  } */

  map<std::string, std::string> ret;
  if (resp.has("Cache-Control")) {
    std::string cacheHeaders = resp.get("Cache-Control");
    LOG(DEBUG) << "cache-control headers=" << cacheHeaders << std::endl;
    if(cacheHeaders == "") {
      return ret;
    }

    std::stringstream ss(cacheHeaders);
    while (ss.good())
    {
        std::string substr;
        getline(ss, substr, ',');

        std::size_t current = substr.find("=");
        if(current != std::string::npos) {
          LOG(DEBUG) << "CC header key=" << substr.substr(0, current) << " value=" << substr.substr(current + 1) << std::endl;
          ret[substr.substr(0, current)] = substr.substr(current + 1);
        } else {
          LOG(DEBUG) << "CC header=" << substr << std::endl;
          ret[substr] = "";
        }
    }

  }

  //resp.add("cacheControlHeaders", ret);
  return ret;
}


// Not cacheable if has directives: no-store, private
// no-cache: may implement this directive, may not, functionality is met but just
// forwarding the response to the client in this case

// must-revalidate makes you re-validate a cached response if it is stale
// no-cache makes you re-validate a cached response everytime
bool ProxyServerCache::isCacheableResp(const Poco::Net::HTTPResponse& resp, std::string id){
  std::map<std::string,std::string> cacheHeaders = getCacheControlHeaders(resp);

  //  LOG(DEBUG) << "cache-control headers=" << cacheHeaders;
    if (cacheHeaders.find("no-store") != cacheHeaders.end()){
      LOG(INFO) << id << ": " << "not cacheable because no-store"<< std::endl;
      return false;
    } else if (cacheHeaders.find("private") != cacheHeaders.end()) {
      LOG(INFO) << id << ": " << "not cacheable because private" << std::endl;
      return false;
    }
  return true;
}


bool ProxyServerCache::hasNoCacheDirective(const Poco::Net::HTTPResponse& resp){
  std::map<std::string,std::string> cacheControlHeaders = getCacheControlHeaders(resp);
  return (cacheControlHeaders.find("no-cache") != cacheControlHeaders.end());
}

// string NO_ETAG indicates no strong validator found
std::string ProxyServerCache::getEtag(const Poco::Net::HTTPResponse& resp){
  if (resp.has("ETag")) {
    return resp.get("ETag");
  }
  return "";
}


// -1 indicates no max-age directive found
double ProxyServerCache::getMaxAge(const Poco::Net::HTTPResponse& resp){
  std::map<std::string,std::string> cacheHeaders = getCacheControlHeaders(resp);
  if (cacheHeaders.find("max-age") != cacheHeaders.end()) {
    std::map<std::string, std::string>::iterator i = cacheHeaders.find("max-age");
    return stod(i->second);
  }
  return -1;
}

// string NO_EXPIRE indicates no expiration header was found
std::string ProxyServerCache::getExpires(const Poco::Net::HTTPResponse& resp){
  if(resp.has("ETag")) {
    return resp.get("ETag");
  }
  return "";
}



std::string ProxyServerCache::getLastModified(const Poco::Net::HTTPResponse& resp){
  if(resp.has("Last-Modified")) {
    return resp.get("Last-Modified");
  }
  return "";
}

CacheResponse::CacheResponse(const Poco::Net::HTTPResponse& response, std::string respData):
  responseData(respData)
{
  Poco::Timestamp ts;
  timeAdded = ts;
  this->responseDate = DateTime(response.getDate());

  //  LOG(TRACE) << "Date cached response was received: " <<
  // now parse to find the Etag, lastModified, and other header fields
  std::string lastModified = ProxyServerCache::getLastModified(response);
  std::string expires = ProxyServerCache::getExpires(response);
  double maxAge = ProxyServerCache::getMaxAge(response);

  if (response.has("Cache-Control")){ // assign cache control values
    string cacheControl = response.get("Cache-Control");
    LOG(TRACE) << "Cache-Control from the HTTPResponse dictionary" << cacheControl << endl;

    // no need to check here for no-store or private, already filtered

    if (cacheControl.find("max-age=") != string::npos){
      this->maxFreshness = atof((cacheControl.substr(cacheControl.find("=") + 1, string::npos)).c_str());
      LOG(TRACE) << "freshness from max-age = " << this->maxFreshness << endl;
    }
    if (cacheControl.find("no-cache") != string::npos){
      this->isNoCache = true;
    }
  }

  else{

    response.has("Expires") ? this->expiresStr = response.get("Expires") : this->expiresStr = "";
    response.has("Last-Modified") ? this->last_modified = response.get("Last-Modified") :
      this->last_modified = "";

    string fmt = "%w, %e %b %Y %H:%M:%S GMT";

    int UTC = 0;

    if (DateTimeParser::tryParse(fmt , this->expiresStr, this->expireDate, UTC));
      // determine freshness lifetime from Expires - Date, or (Date - Last-Modified)/10
  }
  this->Etag = ProxyServerCache::getEtag(response);

  //LOG(DEBUG) << "Copying over headers to cache..." << std::endl;
  ProxyServerCache::copyResponseObj(response, responseObj);
  //LOG(ERROR) << responseObj.get("Cache-Control") << std::endl;
}


bool CacheResponse::isValidResponse(){
  Poco::Timestamp::TimeDiff timeInCache = this->timeAdded.elapsed();
  // timeDiff gives you stuff inmicroseconds
  if (this->isNoCache){
    return false;
  }

  if ((1000 * timeInCache) >= this->maxFreshness){
    LOG(TRACE) << "Item has expired, maxFreshness was: " << this->maxFreshness << ". Time in cache has been: " << (timeInCache/1000) << endl;

    return false;
  }
  LOG(TRACE) << "Item is still fresh " << endl;
  return true;
}


CacheResponse::CacheResponse(const CacheResponse& rhs) :responseData(rhs.responseData.str())
{

  timeAdded = rhs.timeAdded;

  LOG(DEBUG) << "Copying over headers to cache..." << std::endl;
  ProxyServerCache::copyResponseObj(rhs.responseObj, this->responseObj);
}

CacheResponse::~CacheResponse(){}


std::string CacheResponse::getResponseDataStr(){
  return this->responseData.str();
}

std::string CacheResponse::getLastModified(){
  return this->last_modified;
}

std::string CacheResponse::getEtag(){
  return this->Etag;
}

std::ostringstream& CacheResponse::getResponseData(){
  return this->responseData;
}

bool CacheResponse::isExpired(){
  return this->expired;
}

void CacheResponse::setExpired(bool exp){
  this->expired = exp;
}


  void CacheResponse::getResponse(Poco::Net::HTTPResponse& writeToResponse) {
  // copies cookies, headers, etc
  ProxyServerCache::copyResponseObj(responseObj, writeToResponse);
}

/*
double CacheResponse::getTimeLeft(){
  return this->currentFreshness;
}
*/


void CacheResponse::setMaxFreshness(double newFreshness){
  this->maxFreshness = newFreshness;
}


double CacheResponse::getMaxFreshness(){
  return this->maxFreshness;
}

bool CacheResponse::getIsNoCache(){
  return this->isNoCache;
}
