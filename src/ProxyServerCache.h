#include <Poco/UniqueExpireCache.h>
#include <Poco/ExpirationDecorator.h>
#include <Poco/Exception.h>
#include <Poco/LRUCache.h>
#include <ctime>
#include "logging/aixlog.hpp"
#include <Poco/URI.h>
#include <Poco/Clock.h>
#include<Poco/Timestamp.h>

// Reponse object that will be cached with an expiration timer (defuct, not useing ExpireCache)
typedef Poco::ExpirationDecorator<std::istream> ExpRespStream;


class CacheResponse{
private:
  //std::string responseData;
  std::ostringstream responseData;
  double maxFreshness;
  double currentFreshness;
  bool expired;
  Poco::Timestamp timeAdded; // some kind of timer
  std::string Etag;
  std::string last_modified;

public:

  //CacheResponse(std::string respData, double maxFresh, bool exp);
  CacheResponse(const CacheResponse& rhs);

  CacheResponse(std::string respData, double maxFresh, bool exp);
  CacheResponse(std::string respData, double maxFresh, bool exp, std::string Etag);

  std::string getEtag();
  std::string getLastModified();
  
  ~CacheResponse();

  void startExpire(double seconds);

  std::ostringstream& getResponseData();

  std::string getResponseDataStr();

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

  //ProxyServerCache(size_t size);

  ~ProxyServerCache();

  std::string makeKey(Poco::URI& targetURI);

};
