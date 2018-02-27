#include <string>

/* 
   ECE590 HW2 HTTP Proxy 



*/


class Proxy{
  
private:
  int proxy_fd;
  struct addrinfo proxy_info;
  struct addrinfo* proxy_info_list;
  const char * name;
  const char * listen_port;
  
public:
  
  Proxy(){}

};
