#include <string>

struct ParsedReq{
  //const char * URI;
  std::string URI;
  char * host;
  char * port;
  char * reqBuffer; // hold the original http response
  size_t reqBufferLen;
  
  











};
