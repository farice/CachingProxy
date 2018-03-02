#include "ProxyServerCache.h"

using namespace Poco;


// constructor
ProxyServerCache::ProxyServerCache(){
  LOG(DEBUG) << "Yo, the cache is created with size = " << 1024 << std::endl;
}

// constructor (overloaded)
/*
ProxyServerCache::ProxyServerCache(size_t new_size):size(new_size){
  LOG(DEBUG) << "Yo, the cache is created with size = " << this->size << std::endl;
}
*/

// destructor
ProxyServerCache::~ProxyServerCache(){
  LOG(DEBUG) << "Yo, the cache has been destroyed... " << std::endl;
}

std::string ProxyServerCache::makeKey(Poco::URI& uri){
  std::string key(uri.getHost());
  key.append(std::to_string(uri.getPort()));
  key.append(uri.getPathAndQuery());
  LOG(DEBUG) << "Constructed key = " << key << std::endl;
  return key;
}

CacheResponse::CacheResponse(Poco::Net::HTTPResponse response, double maxFresh, bool exp, bool noCache):
  maxFreshness(maxFresh),
  expired(exp),
  isNoCache(noCache)
{
  Poco::Timestamp ts;
  timeAdded = ts;
  ProxyRequestHandler::copyResponseObj(response, responseObj);
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



CacheResponse::CacheResponse(const CacheResponse& rhs):responseData(rhs.responseData.str()),
						       maxFreshness(rhs.maxFreshness),
						       expired(rhs.expired),
						       isNoCache(rhs.isNoCache),
						       timeAdded(rhs.timeAdded),
						       Etag(rhs.Etag),
						       last_modified(rhs.last_modified)
{

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
      LOG(DEBUG) << " Time is up " << std::endl;
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
