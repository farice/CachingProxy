#include <Poco/UniqueExpireCache.h>
#include <Poco/ExpirationDecorator.h>
#include <Poco/Exception.h>
#include <Poco/LRUCache.h>
#include <ctime>
#include "logging/aixlog.hpp"
#include <Poco/URI.h>
#include <Poco/Clock.h>
#include<Poco/Timestamp.h>
#include <Poco/Net/HTTPResponse.h>


// Reponse object that will be cached with an expiration timer (defuct, not useing ExpireCache)
typedef Poco::ExpirationDecorator<std::istream> ExpRespStream;


class CacheResponse{
private:
  //std::string responseData;
  std::ostringstream responseData;
  Poco::Net::HTTPResponse responseObj;
  double maxFreshness;
  //double currentFreshness;
  bool expired;
  bool isNoCache;
  Poco::Timestamp timeAdded; // timestamp of when item was cached
  std::string Etag;
  std::string last_modified;

public:

  static void copyResponseObj(Poco::Net::HTTPResponse &fromResp, Poco::Net::HTTPResponse &toResp);
  static void copyResponseObj(Poco::Net::HTTPResponse &fromResp, Poco::Net::HTTPServerResponse &toResp);
  //CacheResponse(std::string respData, double maxFresh, bool exp);
  CacheResponse(const CacheResponse& rhs);

  CacheResponse(Poco::Net::HTTPResponse response, double maxFresh, bool exp, bool noCache);

  CacheResponse(std::string respData, double maxFresh, bool exp, bool noCache);
  CacheResponse(std::string respData, double maxFresh, bool exp, bool noCache, std::string Etag);
  CacheResponse(std::string respData, double maxFresh, bool exp, bool noCache, std::string Etag,
		std::string last_modified);

  std::string getEtag();
  std::string getLastModified();
  bool getIsNoCache();

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
