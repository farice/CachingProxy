#include "ProxyRequestHandlerFactory.h"
#include "logging/aixlog.hpp"

using namespace Poco::Util;
using namespace std;

namespace Poco {
namespace Net {

ProxyRequestHandlerFactory::ProxyRequestHandlerFactory()
{
  //LOG(DEBUG) << "************* Created ProxyRequestHandlerFactory ************* " << endl;
this->factoryCache = new Poco::LRUCache<std::string, std::string>();
 //LOG(DEBUG) << "!!!!!!!!!!!!!!!!!!!!!! Shared Proxy Cache created !!!!!!!!!!!!!!!!!!!!!!!!" << endl;
}


ProxyRequestHandlerFactory::~ProxyRequestHandlerFactory()
{
  //LOG(DEBUG) << "!!!!!!!!!!!!!!!!!!!!!! Shared Proxy Cache destroyed !!!!!!!!!!!!!!!!!!!!!!!!" << endl;
  delete this->factoryCache;
}


ProxyRequestHandler* ProxyRequestHandlerFactory::createRequestHandler(const HTTPServerRequest &)
  {
    LOG(DEBUG) << "Create request handler" << endl;
    return new ProxyRequestHandler(this->factoryCache);
  }


ProxyRequestHandler* ProxyRequestHandlerFactory::createRequestHandlerWithCache(const HTTPServerRequest &){
//LOG(DEBUG) << "Create request handler with cache" << endl;
return new ProxyRequestHandler(this->factoryCache);
}

}}
