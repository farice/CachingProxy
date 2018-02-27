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
#include <vector>
#include "Proxy.h"



using namespace std;

void Proxy::print_address_sock(const struct sockaddr * address){
  struct sockaddr_in * _address_ = (struct sockaddr_in *) address;
  char * ip = inet_ntoa(_address_->sin_addr);
  cout << "Address = " << ip << endl;
}


// Client may send multiple GET requests even from simply clicking a webpage
// How to handle? You'll have to spawn off threads to do each 


/*
- give the thread the socket fd after the accept call 
- 


*/

void Parser::remove_CR(/*std::*/string& to_remove){
  size_t CR_pos = to_remove.rfind('\r');
  to_remove = to_remove.substr(0, CR_pos);

}



int Proxy::makeServerSocket(const char * address, const char * port){
  int serverSocket;

  struct addrinfo server_hints;
  struct addrinfo* server_info;

  memset(&server_hints, 0, sizeof(server_hints));
  server_hints.ai_family = AF_UNSPEC;
  server_hints.ai_socktype = SOCK_STREAM;

  if (getaddrinfo(address, port, &server_hints, &server_info) != 0){
    perror("getaddrinfo() Proxy::makeServerSocket");
    exit(EXIT_FAILURE);
  }
  
  if ((serverSocket = socket(server_info->ai_family,
			     server_info->ai_socktype,
			     server_info->ai_protocol)) == -1){
    perror("socket() Proxy::makeServerSocket");
    exit(EXIT_FAILURE);
  }

  if (connect(serverSocket, server_info->ai_addr, server_info->ai_addrlen) != 0){
    perror("connect() Proxy::makeServerSocket");
    exit(EXIT_FAILURE);
  }

  free(server_info);
  return serverSocket;
}




int main(int argc, char *argv[]){

  int status;
  int proxy_fd;

  struct addrinfo proxy_info;
  struct addrinfo *proxy_info_list;
  const char *proxyname = NULL;
  const char *listen_port = "8081";

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

  cout << "Proxy waiting for connection on port " << listen_port << endl;

  // NOTE: HTTP requests end with \r\n\r\n

  // Send back 200ok for HTTPS to work

  
  // these are used to store the socket address and get its length
  // for the accpt() call, not sure if we need to keep track of client addresses 
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);

  struct sockaddr client_addr;
  int client_connection_fd;  // file desciptor which can be used to read and write to client


  Proxy p;

  client_connection_fd = accept(proxy_fd, &client_addr, &socket_addr_len);
  if (client_connection_fd == -1) {
    perror("accept()");
    exit(EXIT_FAILURE);
  } 
  cout << "Accepted connection from the following IP:" << endl;
  p.print_address_sock(&client_addr);
  

  /* Thread's should start here (pass in the accepted fd to the thread arg) */

  size_t BUF_SIZE = 1024;
  char * request = (char*) malloc(BUF_SIZE); // get the entire request
  memset(request, 0, sizeof(*request)); 

  size_t recv_total = 0;

  vector<char> message(BUF_SIZE,'\0');
  //vector<char> message(4,'\0');

  
  int it = 0;
  while (strstr(request,"\r\n\r\n") == NULL){  // while the end of the request has not been found

    cout << "---------- iteration number " << it << " ----------" << endl;
    it++;
    
    //char buffer[BUF_SIZE];
    //memset(&buffer, 0, sizeof(buffer));
    
    //ssize_t bytes = recv(client_connection_fd, buffer, sizeof(buffer),0/* MSG_WAITALL*/);
    ssize_t bytes = recv(client_connection_fd, message.data(), (message.size() * sizeof(char)), 0);

    //cout << "after recv" << endl;

    if (bytes == -1){
      perror("recv()");
      exit(EXIT_FAILURE);
    }
    else if (bytes == 0){
      cout << "Shutdown signal, done receiving" << endl;
      break; // exit loop
    }
    else{
      cout << "Number of bytes received = " << bytes << endl;
      recv_total += bytes;

      //buffer[bytes] = '\0'; //null the end character. just checking stuff now
      message[bytes] = '\0';
      
      if (recv_total >= BUF_SIZE){
	BUF_SIZE *= 2; // double the size to amortize costs 
	request = (char*) realloc(request, BUF_SIZE);

	if (request == NULL){
	  perror("failed to allocate memory, realloc()");
	  exit(EXIT_FAILURE);
	}
      }
      
      //strcat(request, buffer); // grow the full request as you get it from recv
      strcat(request, message.data());
      /*
      if (bytes < BUF_SIZE){
	cout << "Sending shutdown" << endl;
	int check = shutdown(client_connection_fd, SHUT_RDWR);
	if (check != 0){
	  perror("Shutdown()");
	  exit(EXIT_FAILURE);
	}
	break; // breaking for now 
      }
      */
    }
  }

  // message has all the data you need to make the request
  // make it 

  // connect to the target server
 
  Parser parser;

  stringstream message_stream;
  message_stream << message.data();
  string parsed_addr;
  string parsed_port; // will be 80 or 8080 for now
  parsed_port = "80"; //"8080";

  getline(message_stream, parsed_addr); // move past first line
  getline(message_stream, parsed_addr, ' '); // move past 2nd line space
  getline(message_stream, parsed_addr); // get remaining

  cout << "The parsed address is (" << parsed_addr << ")" << endl;

  parser.remove_CR(parsed_addr);
  
  cout << "The parsed address is (" << parsed_addr << ")" << endl; // CR should be gone 
  
  
  FILE * f = fopen("request.txt","w");

  if (f == NULL){
    perror("failed to open file for writing");
    exit(EXIT_FAILURE);
  }

  fprintf(f, "%s", request);

  if (fclose(f) != 0){
    perror("failed to close file written to");
    exit(EXIT_FAILURE);
  }
  cout << "************** Server received: **************" << endl;
  cout << request << endl;
  cout << "**********************************************" << endl;
  

  int server_fd = p.makeServerSocket(parsed_addr.c_str(), parsed_port.c_str());
  cout << "returned server fd = " << server_fd << endl;
  
  ssize_t sent = send(server_fd, message.data(), sizeof(char) * message.size(), 0);
  if (sent == -1){
    perror("send()");
    exit(EXIT_FAILURE);
  }
  cout << "send value = " << sent << endl;

  char recv_buff[65000]; // just high number for now 
  memset(&recv_buff, 0, sizeof(recv_buff));
  
  ssize_t bytes = recv(server_fd, recv_buff, sizeof(recv_buff), 0);
  if (bytes == -1){
    perror("recv");
  }
  cout << "Number of bytes received = " << bytes << endl;
  recv_buff[bytes] = '\0';
  
  cout << "Server replied with: " << endl;

  cout << recv_buff << endl;
  /* Parse client request */
		       
  /*
  stringstream req;

  req << request;

  string line;
  getline(req, line);
  cout << "The first line is " << line << endl;
  
  getline(req,line);
  cout << "The second line is " << line << endl;

  string target_name = line.substr(line.find(" ") + 1);
  cout << "The target host name is " << target_name  << endl;

  cout << "Now attempting to connect on behalf of client" << endl;
  */
		       
  
  // free the dynamically allocated linked list of addrinfo structs returned by getaddrinfo
  freeaddrinfo(proxy_info_list);

  free(request);
  
  // close the socket used to accept the message from client
  close(proxy_fd);

  return 0;
}

/*

  char buffer[BUF_SIZE];
  memset(&buffer, 0, sizeof(buffer));

  ssize_t bytes = recv(client_connection_fd, buffer, sizeof(buffer), MSG_WAITALL);
  if (bytes == -1){
    perror("recv()");
    exit(EXIT_FAILURE);
  }
  else if (bytes == 0){
    cerr << "Shutdown signal received" << endl;
    exit(EXIT_FAILURE);
  }
  else{
    cout << "Number of bytes received = " << bytes << endl;
  }
  
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

  
  // Parse client request 

  
  stringstream req;

  req << buffer;

  string line;
  getline(req, line);
  cout << "The first line is " << line << endl;
  
  getline(req,line);
  cout << "The second line is " << line << endl;

  string target_name = line.substr(line.find(" ") + 1);
  cout << "The target host name is " << target_name  << endl;

  cout << "Now attempting to connect on behalf of client" << endl;
*/


  /*
  struct addrinfo target_info;
  struct addrinfo *target_info_list;

  memset(&target_info, 0, sizeof(target_info));
  target_info.ai_family   = AF_UNSPEC;
  target_info.ai_socktype = SOCK_STREAM;

  const char * port_num = "8080"; // 80 or 8080?

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
