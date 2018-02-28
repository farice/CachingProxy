
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

using namespace Poco::Net;

class ProxyRequestHandler : public HTTPRequestHandler
{
public:
  using HTTPRequestHandler::HTTPRequestHandler;

  ProxyRequestHandler();
  ~ProxyRequestHandler();

  virtual void handleRequest(HTTPServerRequest &req, HTTPServerResponse &resp);
private:
  ProxyRequestHandler(const ProxyRequestHandler&);
  ProxyRequestHandler(const HTTPRequestHandler&);
  ProxyRequestHandler& operator = (const ProxyRequestHandler&);

  static int count;
};
