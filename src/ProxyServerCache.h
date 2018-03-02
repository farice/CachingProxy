#include <Poco/UniqueExpireCache.h>
#include <Poco/ExpirationDecorator.h>
#include <Poco/Exception.h>
#include <Poco/LRUCache.h>
#include <ctime>
#include "logging/aixlog.hpp"
#include <Poco/URI.h>

// Reponse object that will be cached with an expiration timer (defuct, not useing ExpireCache)
typedef Poco::ExpirationDecorator<std::istream> ExpRespStream;


class CacheResponse{
private:
  std::string responseData;
  double maxFreshness;
  double currentFreshness;
  bool expired;
  clock_t timeAdded;
  // some kind of timer 
public:

  CacheResponse(std::string respData, double maxFresh, bool exp);

  ~CacheResponse();
  
  void startExpire(double seconds);

  std::string getResponseData();
      
  bool isExpired();
      
  void setExpired(bool exp);

  double getTimeLeft();

  void setMaxFreshness(double newFreshness);

  double getMaxFreshness();

};


class ProxyServerCache : public Poco::LRUCache<std::string, CacheResponse>{
private:

  
public:

  ProxyServerCache();

  ~ProxyServerCache();
  
  std::string makeKey(Poco::URI& targetURI);
  
};

