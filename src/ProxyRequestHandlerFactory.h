#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include "ProxyRequestHandler.h"
#include <Poco/SharedPtr.h>
#include <Poco/UniqueExpireCache.h>
#include <Poco/LRUCache.h>
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
      Poco::LRUCache<std::string, std::string>* factoryCache;
  
  

    };

    /* Instead of having the cache in the factory, you could also have
       it has a static member of the ProxyRequestHandler class */

    ///*

    class CacheResponse{
    private:
      string responseData;
      double  maxFreshness;
      double currentFreshness;
      bool expired;
      // some kind of timer 
    public:

      bool isExpired(){
	return false;
      }

      double getTimeLeft(){
	return 0;
      }

    };


    class CacheResponseTimer{ // timer for determining expiration 
    private:
      double numSeconds;
      bool isExpired;
    public:
      
      CacheResponseTimer(double seconds):numSeconds(seconds), isExpired(false){
	startExpire(this->numSeconds);
      }  

      void startExpire(double seconds){
	clock_t startExpireTime = clock();
	double secondsToExpire = seconds;
	bool stillValid = true;
	double elapsedTime; 
	while (stillValid){
	  elapsedTime = (clock() - startExpireTime) / CLOCKS_PER_SEC;
	  if (elapsedTime >= secondsToExpire){
	    LOG(DEBUG) << " Time is up " << endl;
	    stillValid = false;
	    this->isExpired = true;
	  }
	}
      }


    };

    //*/
 
  }
}



