#include "ProxyServerCache.h"

using namespace Poco;


// constructor
ProxyServerCache::ProxyServerCache(){
  LOG(DEBUG) << "Yo, the cache is created..." << std::endl;
}

// destructor
ProxyServerCache::~ProxyServerCache(){
  LOG(DEBUG) << "You, the cache has been destroyed... " << std::endl;
}

std::string ProxyServerCache::makeKey(Poco::URI& uri){
  std::string key(uri.getHost());
  key.append(std::to_string(uri.getPort()));
  key.append(uri.getPathAndQuery());
  LOG(DEBUG) << "Constructed key = " << key << std::endl;
  return key;
}


CacheResponse::CacheResponse(std::string respData, double maxFresh, bool exp):
  responseData(respData),
  maxFreshness(maxFresh),
  expired(exp),
  timeAdded_t(clock()),
  timeAdded(Poco::Clock())
{
  //startExpire(this->maxFreshness); // start the expiration timer
}

CacheResponse::CacheResponse(const CacheResponse& rhs):responseData(rhs.responseData.str()),
						       maxFreshness(rhs.maxFreshness),
						       currentFreshness(rhs.currentFreshness),
						       expired(rhs.expired),
						       timeAdded(rhs.timeAdded)
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


std::ostringstream& CacheResponse::getResponseData(){
  return this->responseData;
}

bool CacheResponse::isExpired(){
  return this->expired;
}
      
void CacheResponse::setExpired(bool exp){
  this->expired = exp;
}

double CacheResponse::getTimeLeft(){
  return this->currentFreshness;
}

void CacheResponse::setMaxFreshness(double newFreshness){
  this->maxFreshness = newFreshness;
}

double CacheResponse::getMaxFreshness(){
  return this->maxFreshness;
}


