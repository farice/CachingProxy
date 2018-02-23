#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <cstdlib>


using namespace std;

int main(int argc, char *argv[])
{
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;
  const char *port     = "4444";

  // Null's out all the bytes in host_info addrinfo struct
  memset(&host_info, 0, sizeof(host_info));

  // Sets the address family, socket type, and input flags of addrinfo struct
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;
  // There's also a .ai_protocol field of addrinfo struct

  status = getaddrinfo(hostname, port, &host_info, &host_info_list);
  // the second argument (service) is the port number of which is set in each
  // returned addrinfo structure

  // check if the call succeeded
  if (status != 0) {
    cerr << "Error: cannot get address info for host" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } //if

  // the socket call creates a file descriptor for a host endpoint
  // that can be used to communicate
  socket_fd = socket(host_info_list->ai_family,
		     host_info_list->ai_socktype,
		     host_info_list->ai_protocol);
  // check socket call for error
  if (socket_fd == -1) {
    cerr << "Error: cannot create socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } //if


  int yes = 1;

  // SOL_SOCKET specifies SO_REUSEADDR option at the socket level for the socket referenced
  // by the file descriptor, the option_value=yes argument is the value used to set these
  // options for that socket, assuming 1 = set and 0 = don't set(disable)
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  // bind call takes a socket file descripter, a sockaddr struct (which is returned as a member
  // of the addrinfo struct returned by getaddrinfo() call) and a socketlen_t for that
  // sockets address length

  // check the bind call
  if (status == -1) {
    cerr << "Error: cannot bind socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } //if

  // the listen call sets the socket matching the socket_fd as a passive socket
  // that can be used to accept an incoming connection
  status = listen(socket_fd, 100);
  if (status == -1) {
    cerr << "Error: cannot listen on socket" << endl;
    cerr << "  (" << hostname << "," << port << ")" << endl;
    return -1;
  } //if

  cout << "Waiting for connection on port " << port << endl;

  // these are used to store the socket address and get its length
  // for the accpt() call
  struct sockaddr_storage socket_addr;
  socklen_t socket_addr_len = sizeof(socket_addr);

  struct sockaddr client_addr;
  int client_connection_fd;  // file desciptor which can be used to read and write to client

  client_connection_fd = accept(socket_fd, &client_addr, &socket_addr_len);
  // the accept call extracts the first connection request from the queue of pending
  // connections for the socket named by socket_fd, it then creates a new socket and
  // returns the file descriptor referring to that socket (not in listening state yet)

  if (client_connection_fd == -1) {
    cerr << "Error: cannot accept connection on socket" << endl;
    return -1;
  } //if

  char buffer[1024];

  // receives messages from socket client_connection_fd and stores it in the
  // supplied buffer, the length of the message is returned on success,

  // if no data is available on the socket then this call waits, unless it is non-blocking
  // in which case -1 is returned
  recv(client_connection_fd, buffer, sizeof(buffer), MSG_WAITALL);

  buffer[1023] = 0; //null the end character. just checking stuff now

  FILE * f = fopen("request.txt","w");

  //int daemon_status = daemon(0,0);
  if (f == NULL){
    perror("failed to open file for writing");
    exit(EXIT_FAILURE);
  }

  fprintf(f, "%s\n", buffer);

  if (fclose(f) != 0){
    perror("failed to close file written to");
    exit(EXIT_FAILURE);
  }

  cout << "Server received: " << buffer << endl;

  // free the dynamically allocated linked list of addrinfo structs returned by getaddrinfo
  freeaddrinfo(host_info_list);

  // close the socket used to accept the message from client
  close(socket_fd);

  return 0;
}
