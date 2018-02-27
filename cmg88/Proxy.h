#include <string>

/* 
   ECE590 HW2 HTTP Proxy 



*/



// Client Request object
class ClientRequest{
private:
  char * method; // likely unnecessary, request will always be GET
  char * protocol; // likely unnecessary, only using HTTP
  char * clientHost;
  char * port;
  char * path;
  char version[10];
  char * raw; // raw http request as received in buffer
  size_t raw_len;
  
  // how much of this is actually used ?
  // More important to make a server request object from a client request 
public:
  

};

// Client Response object
class ClientResponse{
private:




public:


};

// Server Request object
class ServerRequest{
private:
  char * method; // likely unnecessary, request will always be GET
  char * protocol; // likely unnecessary, only using HTTP
  char * host; // server endhost
  char * port; // server port
  char * path; // path portion of URI (path to resource + resource)
  char version[10];
  char * raw; // raw http request as received in buffer
  size_t raw_len; // number of bytes received in the buffer
  
public:
  

};

// Server Response object 


class Proxy{
  
private:
  int proxy_fd;
  struct addrinfo proxy_info;
  struct addrinfo* proxy_info_list;
  const char * name;
  const char * listen_port;
  
public:
  
  Proxy(){}

  int makeServerSocket(const char * address, const char* port);

  void print_address_sock(const struct sockaddr * address);
};

class Parser{
private:
  // nothing at the moment

public:
  void remove_CR(std::string& to_remove);






};
