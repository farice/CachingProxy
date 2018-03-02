
#include<Poco/Net/HTTPServerConnectionFactory.h>
#include <Poco/Net/SSLManager.h>
#include <Poco/Net/SSLException.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/AcceptCertificateHandler.h>
#include <Poco/Net/HTTPSClientSession.h>
#include <Poco/StreamCopier.h>
#include <Poco/URI.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Util/ServerApplication.h>
#include <Poco/Exception.h>

#include <Poco/UniqueExpireCache.h>
#include <Poco/LRUCache.h>
#include <Poco/ExpirationDecorator.h>


using namespace Poco::Net;

// Expiration wrapper for istream responses stored in the cache
typedef Poco::ExpirationDecorator<std::string> ExpRespStream;


class ProxyRequestHandler : public HTTPRequestHandler
{
public:
  using HTTPRequestHandler::HTTPRequestHandler;

  ProxyRequestHandler(Poco::LRUCache<std::string, std::string>* cache);
  ProxyRequestHandler();
  ~ProxyRequestHandler();

  virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp);
private:
  ProxyRequestHandler(const ProxyRequestHandler&);
  ProxyRequestHandler(const HTTPRequestHandler&);
  ProxyRequestHandler& operator = (const ProxyRequestHandler&);

  // Cache for the request handler (may have to move up hierarchy to
  // have caching span multiple request or even connections)

  //Poco::UniqueExpireCache<std::string, ExpRespStream> requestCache;
  Poco::LRUCache<std::string, std::string>* requestCache;
  
  static int count;
};
