#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include "ProxyRequestHandler.h"
#include <Poco/SharedPtr.h>
#include <Poco/ExpirationDecorator.h>
#include <ctime>
#include "logging/aixlog.hpp"

using namespace std;

namespace Poco {
  namespace Net {

    class ProxyRequestHandlerFactory : public HTTPRequestHandlerFactory
    {
    public:
      typedef Poco::SharedPtr<ProxyRequestHandlerFactory> Ptr;

      ProxyRequestHandlerFactory();
      ~ProxyRequestHandlerFactory();
      virtual ProxyRequestHandler* createRequestHandler(const HTTPServerRequest &);
      virtual ProxyRequestHandler* createRequestHandlerWithCache(const HTTPServerRequest &);

      Poco::BasicEvent<const bool> serverStopped;

      /* -Could use expire cache, but may prevent re-validation functionality
	 -Using LRU cache for now
      */
      //Poco::LRUCache<std::string, std::string>* factoryCache;
      //Poco::LRUCache<std::string, CacheResponse>* factoryCache;

      // COULD BE A NAMESPACI

      ProxyServerCache * factoryCache;
    };

    /* Instead of having the cache in the factory, you could also have
       it has a static member of the ProxyRequestHandler class */

    ///*

  }
}
