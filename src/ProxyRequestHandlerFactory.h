#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include "ProxyRequestHandler.h"
#include <Poco/SharedPtr.h>

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
  ProxyRequestHandler* createProxyRequestHandler(const HTTPServerRequest &);
  Poco::BasicEvent<const bool> serverStopped;
};

}}
