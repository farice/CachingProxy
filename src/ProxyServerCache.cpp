#include "ProxyServerCache.h"

using namespace Poco;;
using namespace std;

// constructor
ProxyServerCache::ProxyServerCache(){
  //LOG(DEBUG) << "Yo, the cache is created with size = " << 1024 << std::endl;
}

// constructor (overloaded)
/*
ProxyServerCache::ProxyServerCache(size_t new_size):size(new_size){
  //LOG(DEBUG) << "Yo, the cache is created with size = " << this->size << std::endl;
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
  //LOG(DEBUG) << "Constructed key = " << key << std::endl;
  return key;
}


void ProxyServerCache::copyResponseObj(const Poco::Net::HTTPResponse &fromResp, Poco::Net::HTTPResponse &toResp) {
  // Copy over headers
  Poco::Net::NameValueCollection::ConstIterator i = fromResp.begin();
  while(i!=fromResp.end()){
      ////LOG(DEBUG) << "Copying over header=" <<  i->first << std::endl;
      toResp.add(i->first, i->second);
      ////LOG(DEBUG) << "Copying over value=" <<  toResp.get(i->first) << std::endl;
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
      ////LOG(DEBUG) << "Copying over header=" <<  i->first << std::endl;
      toResp.add(i->first, i->second);
      ////LOG(DEBUG) << "Copying over value=" <<  toResp.get(i->first) << std::endl;
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

CacheResponse::CacheResponse(const Poco::Net::HTTPResponse& response, std::string respData):
  responseData(respData)
{
  Poco::Timestamp ts;
  timeAdded = ts;

  if (response.has("Cache-Control")){ // assign cache control values
    string cacheControl = response.get("Cache-Control");
    LOG(TRACE) << "Cache-Control from the HTTPResponse dictionary" << cacheControl << endl;

    // no need to check here for no-store or private, already filtered

    if (cacheControl.find("max-age=") != string::npos){
      this->maxFreshness = atof((cacheControl.substr(cacheControl.find("=") + 1, string::npos)).c_str());
      LOG(TRACE) << "freshness from max-age = " << this->maxFreshness << endl;
    }
    else{
      this->maxFreshness = 10;
      //   response.has("Expires") ? this
      // determine freshness lifetime from Expires - Date, or (Date - Last-Modified)/10
    }
  }
  response.has("ETag") ? this->Etag = response.get("ETag") : this->Etag = "";
  response.has("Last-Modified") ? this->last_modified = response.get("Last-Modified") :
    this->last_modified = "";
  
  
  ////LOG(DEBUG) << "Copying over headers to cache..." << std::endl;
  ProxyServerCache::copyResponseObj(response, responseObj);
  //LOG(ERROR) << responseObj.get("Cache-Control") << std::endl;

  //startExpire(this->maxFreshness); // start the expiration timer
}


bool CacheResponse::isValidResponse(){
  Poco::Timestamp::TimeDiff timeInCache = this->timeAdded.elapsed();
  // timeDiff gives you stuff inmicroseconds 
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

  //LOG(DEBUG) << "Copying over headers to cache..." << std::endl;
  ProxyServerCache::copyResponseObj(rhs.responseObj, this->responseObj);

  //startExpire(this->maxFreshness); // start the expiration timer
}


CacheResponse::CacheResponse(std::string respData, double maxFresh, bool exp, bool noCache):
  responseData(respData),
  maxFreshness(maxFresh),
  expired(exp),
  isNoCache(noCache)
{
  Poco::Timestamp ts;
  timeAdded = ts;
  //startExpire(this->maxFreshness); // start the expiration timer
}

CacheResponse::CacheResponse(std::string respData, double maxFresh, bool exp, bool noCache,
			     std::string Etag):
  responseData(respData),
  maxFreshness(maxFresh),
  expired(exp),
  isNoCache(noCache),
  Etag(Etag)
{
  Poco::Timestamp ts;
  timeAdded = ts;
  //startExpire(this->maxFreshness); // start the expiration timer
}

CacheResponse::CacheResponse(std::string respData, double maxFresh, bool exp, bool noCache, std::string Etag, std::string last_modified):responseData(respData),maxFreshness(maxFresh),expired(exp), isNoCache(noCache),Etag(Etag),last_modified(last_modified)
{
  Poco::Timestamp ts;
  timeAdded = ts;
  //startExpire(this->maxFreshness); // start the expiration timer
}

CacheResponse::~CacheResponse(){}


void CacheResponse::startExpire(double seconds){
  clock_t startExpireTime = clock();
  double secondsToExpire = seconds;
  bool stillValid = true;
  double elapsedTime;
  while (stillValid){ // right now this just waits till its expired, retard
    elapsedTime = (clock() - startExpireTime) / CLOCKS_PER_SEC;
    if (elapsedTime >= secondsToExpire){
      //LOG(DEBUG) << " Time is up " << std::endl;
      stillValid = false;
      this->expired = true;
    }
  }
}


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
