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
#include <Poco/Util/ServerApplication.h>
#include <Poco/Exception.h>
#include "ProxyServerCache.h"

using namespace Poco::Net;


class ProxyRequestHandler : public HTTPRequestHandler
{
public:
  using HTTPRequestHandler::HTTPRequestHandler;

  //ProxyRequestHandler(ProxyServerCache* cache); // now using static member

  ProxyRequestHandler();

  ~ProxyRequestHandler();

  static void copyResponseObj(Poco::Net::HTTPResponse &fromResp, Poco::Net::HTTPResponse &toResp);

  void updateCacheItem(Poco::SharedPtr<CacheResponse>  item);

  virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp);

  std::vector<std::pair<std::string,std::string> > getHeaders(HTTPResponse& resp);

  std::vector<std::pair<std::string,std::string> > getCacheControlHeaders(HTTPResponse& resp);

  /*
  // trying to get an item from the cache and check its time to see if valid
  Poco::SharedPtr<CacheResponse> checkAndGetResponse(std::string key);
  // returns nullptr if the item is not valid or not present
  // we'll need to differentiate between not present and needing re-validation, etc.
  */ // not sure about this
  
  // either store the header value as a string or a timestamp
  //std::string getDate(HTTPResponse& resp);
  // update: can use the built-in function for getting date

 
  double getMaxAge(HTTPResponse& resp);

  std::string getExpires(HTTPResponse& resp);
  
  std::string getEtag(HTTPResponse& resp);
  
  std::string getLastModified(HTTPResponse& resp);
  
  bool isCacheableResp(HTTPResponse& resp);

  bool hasNoCacheDirective(HTTPResponse& resp);
  
  
private:
  ProxyRequestHandler(const ProxyRequestHandler&);
  ProxyRequestHandler(const HTTPRequestHandler&);
  ProxyRequestHandler& operator = (const ProxyRequestHandler&);

  // Cache for the request handler (may have to move up hierarchy to
  // have caching span multiple request or even connections)

  //ProxyServerCache * requestCache; // defunct, no longer passing in from factory

  static ProxyServerCache staticCache;

  static int count;
};
