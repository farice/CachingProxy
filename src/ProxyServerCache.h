#include <Poco/UniqueExpireCache.h>
#include <Poco/ExpirationDecorator.h>
#include <Poco/Exception.h>
#include <Poco/LRUCache.h>
#include <ctime>
#include "logging/aixlog.hpp"
#include <Poco/URI.h>
#include <Poco/Clock.h>
#include <Poco/Timestamp.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/DateTimeFormatter.h>
#include <Poco/DateTimeFormat.h>

// Reponse object that will be cached with an expiration timer (defuct, not useing ExpireCache)
typedef Poco::ExpirationDecorator<std::istream> ExpRespStream;


class CacheResponse{
private:
  //std::string responseData;
  std::ostringstream responseData;

  Poco::DateTime responseDate;
  Poco::DateTime expireDate;
  Poco::DateTime LastModifiedDate;
  
  Poco::Net::HTTPResponse responseObj;
  double maxFreshness;
  //double currentFreshness;
  bool expired;
  bool isNoCache;
  Poco::Timestamp timeAdded; // timestamp of when item was cached
  std::string expiresStr;
  std::string Etag;
  std::string last_modified;

public:

  //CacheResponse(std::string respData, double maxFresh, bool exp);
  CacheResponse(const CacheResponse& rhs);

  CacheResponse(const Poco::Net::HTTPResponse& response, std::string respData);

  CacheResponse(std::string respData, double maxFresh, bool exp, bool noCache);
  CacheResponse(std::string respData, double maxFresh, bool exp, bool noCache, std::string Etag);
  CacheResponse(std::string respData, double maxFresh, bool exp, bool noCache, std::string Etag,
		std::string last_modified);

  std::string getEtag();
  std::string getLastModified();
  bool getIsNoCache();

  bool isValidResponse();
  
  ~CacheResponse();

  std::ostringstream& getResponseData();
  void getResponse(Poco::Net::HTTPResponse& writeToResponse);

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
  static void copyResponseObj(const Poco::Net::HTTPResponse &fromResp, Poco::Net::HTTPResponse &toResp);
  static void copyResponseObj(const Poco::Net::HTTPResponse &fromResp, Poco::Net::HTTPServerResponse &toResp);

  ProxyServerCache();

  ~ProxyServerCache();

  std::string makeKey(Poco::URI& targetURI);

};
