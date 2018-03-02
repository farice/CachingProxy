#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include "ProxyRequestHandler.h"
#include <Poco/SharedPtr.h>
#include <Poco/UniqueExpireCache.h>
#include <Poco/LRUCache.h>
#include <Poco/ExpirationDecorator.h>
#include <ctime>

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

/*
 class CacheResponse{
   string responseData;
   int maxFreshness;
   int currentFreshness;
   bool isExpired;
   // some kind of timer 
   




 };


 class CacheResponseTimer{ // timer for determining expiration 
   double numSeconds;
   bool isExpired;
 public:

 CacheResponseTimer(double seconds):numSeconds(seconds), isExpired(false){
   
   void startExpire(double seconds){
     clock_t startExpireTime = clock();
     double secondsToExpire = seconds;
     bool stillValid = true;
     double elapsedTime; 
     while (stillValid){
       elapsedTime = (clock() - startExpireTime) / CLOCKS_PER_SEC;
       if (elapsedTime >= secondsToExpire){
	 LOG(TRACE) << " Time is up " << endl;
	 stillValid = false;
	 this->isExpired = true;
       }
     }
   }


 };

*/
 
}}



