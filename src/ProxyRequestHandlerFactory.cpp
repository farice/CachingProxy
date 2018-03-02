#include "ProxyRequestHandlerFactory.h"
#include "logging/aixlog.hpp"

namespace Poco {
  namespace Net {

    ProxyRequestHandlerFactory::ProxyRequestHandlerFactory()
    {
      //LOG(TRACE) << "************* Created ProxyRequestHandlerFactory ************* " << endl;
      //this->factoryCache = new ProxyServerCache(); // defunct
      //LOG(TRACE) << "Shared Proxy Cache created " << endl;
    }


    ProxyRequestHandlerFactory::~ProxyRequestHandlerFactory()
    {
      //LOG(TRACE) << " Shared Proxy Cache destroyed " << endl;
      //delete this->factoryCache; // defunct
    }


    ProxyRequestHandler* ProxyRequestHandlerFactory::createRequestHandler(const HTTPServerRequest&)
    {
      //LOG(TRACE) << "Create request handler" << endl;
      return new ProxyRequestHandler();
    }
  }
}

