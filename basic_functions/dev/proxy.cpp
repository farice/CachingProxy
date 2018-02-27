#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>
#include <sstream>
#include <netinet/in.h>
#include <arpa/inet.h>


using namespace std;

void print_address_sock(const struct sockaddr * address){
  struct sockaddr_in * _address_ = (struct sockaddr_in *) address;
  char * ip = inet_ntoa(_address_->sin_addr);
  cout << "Address = '%s'\n" <<  ip << endl;
}


// Client may send multiple GET requests even from simply clicking a webpage
// How to handle? You'll have to spawn off threads to do each 


/*
- give the thread the socket fd after the accept call 
- 


*/




int main(int argc, char *argv[]){

  int status;
  int proxy_fd;

  struct addrinfo proxy_info;
  struct addrinfo *proxy_info_list;
  const char *proxyname = NULL;
  const char *listen_port = "8080";

  memset(&proxy_info, 0, sizeof(proxy_info)); // init

  proxy_info.ai_family   = AF_UNSPEC;
  proxy_info.ai_socktype = SOCK_STREAM;
  proxy_info.ai_flags    = AI_PASSIVE;

  status = getaddrinfo(proxyname, listen_port, &proxy_info, &proxy_info_list);
  if (status != 0) {
    perror("getaddrinfo() for proxy setup");
    exit(EXIT_FAILURE);
  }

  proxy_fd = socket(proxy_info_list->ai_family,
		    proxy_info_list->ai_socktype,
		    proxy_info_list->ai_protocol);
  if (proxy_fd == -1) {
    perror("socket() call for proxy setup");
    exit(EXIT_FAILURE);
  } 

  int yes = 1;
  status = setsockopt(proxy_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  status = bind(proxy_fd, proxy_info_list->ai_addr, proxy_info_list->ai_addrlen);
  if (status == -1) {
    perror("bind() for proxy setup");
    exit(EXIT_FAILURE);
  } 

  status = listen(proxy_fd, 100); // number of pending connections 
  if (status == -1) {
    perror("listen() for proxy setup");
    exit(EXIT_FAILURE);
  } 

  cout << "Waiting for connection on port " << listen_port << endl;





  
  // these are used to store the socket address and get its length
  // for the accpt() call, not sure if we need to keep track of client addresses 
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);

  struct sockaddr client_addr;
  int client_connection_fd;  // file desciptor which can be used to read and write to client



  client_connection_fd = accept(proxy_fd, &client_addr, &socket_addr_len);


  /* Thread's should start here */

  
  if (client_connection_fd == -1) {
    perror("accept()");
    exit(EXIT_FAILURE);
  } 

  char buffer[1024];

  // receives messages from socket client_connection_fd and stores it in the
  // supplied buffer, the length of the message is returned on success,

  // if no data is available on the socket then this call waits, unless it is non-blocking
  // in which case -1 is returned
  recv(client_connection_fd, buffer, sizeof(buffer), MSG_WAITALL);

  buffer[1023] = '\0'; //null the end character. just checking stuff now

  FILE * f = fopen("request.txt","w");

  if (f == NULL){
    perror("failed to open file for writing");
    exit(EXIT_FAILURE);
  }

  fprintf(f, "%s", buffer);

  if (fclose(f) != 0){
    perror("failed to close file written to");
    exit(EXIT_FAILURE);
  }

  cout << "Server received: " << buffer << endl;

  stringstream req;

  req << buffer;

  string line;
  getline(req, line);
  cout << "The first line is " << line << endl;

  getline(req,line);
  cout << "The second line is " << line << endl;

  string target_name = line.substr(line.find(" ") + 1);
  cout << "The target host name is (" << target_name << ")" << endl;
  
  struct addrinfo target_info;
  struct addrinfo *target_info_list;

  memset(&target_info, 0, sizeof(target_info));
  target_info.ai_family   = AF_UNSPEC;
  target_info.ai_socktype = SOCK_STREAM;

  const char * port_num = "8080"; // 80 or 8080?

  /*
  // replace with the address of the server the client requested
  // you'll have to do some parsing of the GET request 
  status = getaddrinfo(target_name, port_num, &target_info, &target_info_list);
  
  if (status != 0) {
    perror("getaddrinfo()");
    exit(EXIT_FAILURE);
  }

  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
 
  if (socket_fd == -1) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }
  
  //printf("Connecting to ringmaster %s on port %s\n",ringmaster_name, port_num);
  
  status = connect(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    perror("connect()");
    exit(EXIT_FAILURE);
  }

  */

  
  // free the dynamically allocated linked list of addrinfo structs returned by getaddrinfo
  freeaddrinfo(proxy_info_list);

  // close the socket used to accept the message from client
  close(proxy_fd);

  return 0;
}
