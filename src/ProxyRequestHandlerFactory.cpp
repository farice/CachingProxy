#include "ProxyRequestHandlerFactory.h"
#include "logging/aixlog.hpp"

using namespace Poco::Util;
using namespace std;

namespace Poco {
namespace Net {

ProxyRequestHandlerFactory::ProxyRequestHandlerFactory()
{
}


ProxyRequestHandlerFactory::~ProxyRequestHandlerFactory()
{
}

ProxyRequestHandler* ProxyRequestHandlerFactory::createRequestHandler(const HTTPServerRequest &)
  {
    LOG(INFO) << "Create request handler" << endl;
    return new ProxyRequestHandler;
  }

}}
