#include "potato.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <time.h>

// Range of ports open 
#define MAX_PORT 51097
#define MIN_PORT 51015

// Helper print functions used for debugging 
void print_address_addr(struct addrinfo * host){
  const struct sockaddr * address_to_connect = host->ai_addr;
  struct sockaddr_in * address_to_connect_in =
    (struct sockaddr_in *) address_to_connect;
  char * ip = inet_ntoa(address_to_connect_in->sin_addr);
  printf("Address = '%s'\n", ip);
}


void print_address_sock(const struct sockaddr * address){
  struct sockaddr_in * _address_ = (struct sockaddr_in *) address;
  char * ip = inet_ntoa(_address_->sin_addr);
  printf("Address = '%s'\n", ip);
}

/* USAGE:
 *
 * invoke process as: ./player <machine_name> <port_num>
 * machine_name = machine running ringmaster process
 * port_num = port used by ringmaster to communicate with players
 * 
 */
int main(int argc, char* argv[]){
  // Validate command line arguments:               
  if (argc != 3){
    fprintf(stderr, "Error: usage is ./player <machine_name> <port_num>\n");
    exit(EXIT_FAILURE);
  }
  if ((atoi(argv[2]) < 0) || (atoi(argv[2]) > 65535)){
    fprintf(stderr, "Error: please choose a valid port number\n");
    exit(EXIT_FAILURE);
  }

  const char *ringmaster_name = argv[1];
  const char * port_num = argv[2];
  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;

  memset(&host_info, 0, sizeof(host_info));
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;

  status = getaddrinfo(ringmaster_name, port_num, &host_info, &host_info_list);
  
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

  response player_response;
  memset(&player_response, 0, sizeof(player_response));
  int listen_socket;

  struct addrinfo player_info;
  struct addrinfo* player_info_list;
  const char * player_name = NULL;

  // find port to listen on
  for (int i = MIN_PORT; i <= MAX_PORT; i++){

    memset(&player_info, 0, sizeof(player_info));
    
    player_info.ai_family   = AF_UNSPEC;
    player_info.ai_socktype = SOCK_STREAM;
    player_info.ai_flags    = AI_PASSIVE;
    
    char port[6];
    snprintf(port, 6, "%d", i);
    
    status = getaddrinfo(player_name, port, &player_info, &player_info_list);
    if (status != 0){
      perror("getaddrinfo() search for port");
      exit(EXIT_FAILURE);
    }
    
    listen_socket = socket(player_info_list->ai_family, 
			   player_info_list->ai_socktype, 
			   player_info_list->ai_protocol);
    if (listen_socket == -1){
      printf("listen socket\n");
    }
    
    // this sequence results in '0.0.0.0' for player_info_list->ai_addr
    // this is the same as IPADDR_ANY
    status = bind(listen_socket, player_info_list->ai_addr, 
		  player_info_list->ai_addrlen);
      
    if (status == -1) {
      if (i == MAX_PORT){
	fprintf(stderr, "No port available for player to listen\n");
	freeaddrinfo(player_info_list);
	freeaddrinfo(host_info_list);
	exit(EXIT_FAILURE);
      }

      freeaddrinfo(player_info_list);
      continue;
    }
 
    if ((status = listen(listen_socket, 1) == 0)){ // success on port
      player_response.player_id = -1;
      snprintf(player_response.port_num, 6, "%d", i);
      break;
    }
  }

  // confirm player has listening sockets by sending the found port
  ssize_t bytes = send(socket_fd, &player_response, sizeof(player_response), 0);
  if (bytes == -1){
    perror("send() ready message");
    exit(EXIT_FAILURE);
  }
  
  // retrieve setup info
  player_setup setup;
  bytes = recv(socket_fd, &setup, sizeof(setup), MSG_WAITALL);
  if (bytes == -1){
    perror("recv() setup info");
    exit(EXIT_FAILURE);
  }

  /*
  // For verification/debugging
  printf("Player received transmission of %li bytes\n", bytes);
  printf("Player id = %d\n", setup.player_id);
  printf("Right id = %d\n", setup.right_id);
  printf("Left id = %d\n", setup.left_id);
  printf("Player address:\n");
  print_address_sock(&setup.player_addr);
  printf("Right address:\n");
  print_address_sock(&setup.right_addr);
  printf("Right port: %s\n", setup.right_port);
  printf("Left address:\n");
  print_address_sock(&setup.left_addr);
  */

  int player_id = setup.player_id;
  int num_players = setup.total_players;

  // create right player connection   
  int right_socket;
  
  const struct sockaddr * rt = &setup.right_addr;
  struct sockaddr_in * ri = (struct sockaddr_in *) rt;  
  const char * right_name = inet_ntoa(ri->sin_addr);
  
  struct addrinfo right_info;
  struct addrinfo* right_info_list;
  memset(&right_info, 0, sizeof(right_info));

  right_info.ai_family = AF_UNSPEC;
  right_info.ai_socktype = SOCK_STREAM;
	     
  status = getaddrinfo(right_name, setup.right_port, &right_info, &right_info_list);
  if (status != 0){
    perror("getaddrinfo() right player");
    exit(EXIT_FAILURE);
  }

  right_socket = socket(right_info_list->ai_family, 
			right_info_list->ai_socktype, 
			right_info_list->ai_protocol);
  
  if (right_socket == -1){
    perror("right socket()");
    exit(EXIT_FAILURE);
  }

  player_response.player_id = player_id;

  bytes = send(socket_fd, &player_response, sizeof(player_response), 0); 
  // ^ once sent the ringmaster sets up the next player 

  if (bytes == -1){
    perror("send() ready message");
    exit(EXIT_FAILURE);
  }

  int left_socket;

  /* connect right then accept left (b/c accpet will block) */

  // connect right
  //printf("Connecting to %s on port %s\n", right_name, setup.right_port);
  status = connect(right_socket, right_info_list->ai_addr, right_info_list->ai_addrlen);  
  if (status == -1) {
    perror("connect() right socket");
    exit(EXIT_FAILURE);
  }
  
  // accept left 
  //printf("Accepting left player connection\n");
  left_socket = accept(listen_socket, NULL, NULL); // don't need other fields IP already known
  if (left_socket == -1){
    perror("accept() left player connection");
    exit(EXIT_FAILURE);
  }
  
  // player is now fully connected
  printf("Connected as player %d out of %d total players\n", player_id, num_players);

  
  // create fd set to monitor for potato
  fd_set game_fds;
  
  FD_ZERO(&game_fds);
  
  FD_SET(socket_fd, &game_fds);
  FD_SET(left_socket, &game_fds);
  FD_SET(right_socket, &game_fds);
  
  int reading_fd;
  int direction;
  
  potato buffer[2];
  memset(&buffer[0], 0, sizeof(buffer[0]));
  memset(&buffer[1], 0, sizeof(buffer[1]));
  
  srand(time(NULL) + player_id);

  /* The game begins... */
  while (1){

    direction = rand() % 2; // 0 or 1 for left or right
    
    //printf("------------------------------------------------------\n");
    
    // block until input arrives from one of the connections 
    if (select(left_socket + 1, &game_fds, NULL, NULL, NULL) == -1){
      perror("select()");
    }
    
    // check which fd
    if (FD_ISSET(socket_fd, &game_fds)){
      reading_fd = socket_fd;
      //printf("Received from ringmaster\n");
    }
    else if (FD_ISSET(left_socket, &game_fds)){
      reading_fd = left_socket;
      //printf("Received from left player %d\n", setup.left_id);
    }
    else if (FD_ISSET(right_socket, &game_fds)){
      reading_fd = right_socket;
      //printf("Received from right player %d\n", setup.right_id);
    }
    else{
      fprintf(stderr,"Unknown fd error\n");
      exit(EXIT_FAILURE);
    }
    
    
    bytes = recv(reading_fd, buffer, sizeof(buffer), MSG_WAITALL);
    //printf("%li bytes received\n", bytes);
    
    if (bytes == -1){
      perror("recv in game");
      exit(EXIT_FAILURE);
    }
    
    // if recv returns a value of 0, the socket peer has performed 
    // an orderly shutdown, the player process should terminate 
    if (bytes == 0){
      // release allocated memory and close fds below loop
      break; 
    }

    potato p = buffer[0];
    
    //printf("Player %d received potato with %d num_hops\n", player_id, p.num_hops);
    //printf("Writing in player id %d array at position %d\n", player_id, p.current_hop);

    p.player_ids[p.current_hop] = player_id; // append player id
    p.current_hop++; // set for next player
    p.num_hops--; // decrement hops
  
    buffer[0] = p;

    //printf("Now potato has %d num_hops and current hop = %d\n", 
    //buffer[0].num_hops, buffer[0].current_hop);

    if (p.num_hops <= 0){ // game over (covers num_hops intialized to 0 case)
      printf("I'm it\n");

      bytes = send(socket_fd, buffer, sizeof(buffer), 0); // send to ringmaster to end
      if (bytes == -1){
	perror("send() game end");
	exit(EXIT_FAILURE);
      }
    }
    else{
      if (direction){ // send right
	printf("Sending potato to %d\n", setup.right_id);	

	bytes = send(right_socket, buffer, sizeof(buffer), 0);
	if (bytes == -1){
	  perror("send() potato right");
	  exit(EXIT_FAILURE);
	}
      }
      else{ // send left

	printf("Sending potato to %d\n", setup.left_id);  

	bytes = send(left_socket, buffer, sizeof(buffer), 0);
	if (bytes == -1){
	  perror("send() potato left");
	  exit(EXIT_FAILURE);
	}
      }
    }
    // reset fds for select call
    FD_ZERO(&game_fds);
    FD_SET(socket_fd, &game_fds);
    FD_SET(right_socket, &game_fds);
    FD_SET(left_socket, &game_fds);
    
    //printf("------------------------------------------------------\n");
  }

  close(socket_fd);
  close(left_socket);
  close(right_socket);
  close(listen_socket);

  freeaddrinfo(host_info_list);
  freeaddrinfo(right_info_list);
  freeaddrinfo(player_info_list);

  return EXIT_SUCCESS;
}



