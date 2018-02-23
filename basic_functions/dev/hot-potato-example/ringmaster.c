#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include "potato.h"

/************************************************
 * ECE650 -- Homwork#3 -- Connor Grehlinger     *
 * Hot Potato -- 02/13/18                       *
 ************************************************/


/* This struct holds the player attributes necessary for
 * the ringmaster to communicate with each player */
typedef struct player_info_t{
  int player_id;
  char is_connected;
  int socket;   
  
  struct sockaddr player_address;
  socklen_t address_len;
  char port[6];

  player_setup * player_setup;
  
} player_info;


// Helper functions for debugging 
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
 * invoke process as: ./ringmaster <port_num> <num_players> <num_hops>
 *
 * port_num = port for ringmaster to communicate with players 
 * num_players = number of players in hot potato game 
 * num_hops = number of hops the potato takes in game */

int main(int argc, char* argv[]){
  // Validate command line arguments
  if (argc != 4){
    fprintf(stderr, "Error: usage is ./ringmaster <port_num> <num_players> <num_hops>\n");
    exit(EXIT_FAILURE);
  }
  if ((atoi(argv[1]) < 0) || (atoi(argv[1]) > 65535)){
    fprintf(stderr, "Error: please choose a valid port number\n");
    exit(EXIT_FAILURE);
  }
  if(atoi(argv[2]) <= 1){
    fprintf(stderr, "Error: num_players must be greater than 1\n");
    exit(EXIT_FAILURE);
  }
  if((atoi(argv[3]) < 0) || (atoi(argv[3]) > 512)){
    fprintf(stderr, "Error: num_hops must be in the range [0-512]\n");
    exit(EXIT_FAILURE);
  }
  const char* port_num = argv[1];
  const int num_players = atoi(argv[2]);
  const unsigned int num_hops = atoi(argv[3]);

  // Initial output
  printf("Potato Ringmaster\n");
  printf("Players = %u\n", num_players);
  printf("Hops = %u\n", num_hops);

  // Initialize potato
  potato hot_potato = {.num_hops = num_hops, .current_hop = 0};

  int status;
  int socket_fd;
  struct addrinfo host_info;
  struct addrinfo *host_info_list;
  const char *hostname = NULL;

  memset(&host_info, 0, sizeof(host_info)); 

  // Sets the address family, socket type, and input flags of addrinfo struct
  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;

  status = getaddrinfo(hostname, port_num, &host_info, &host_info_list);
  if (status != 0) {
    perror("getaddrinfo()");
    exit(EXIT_FAILURE);
  }

  socket_fd = socket(host_info_list->ai_family, host_info_list->ai_socktype, 
		     host_info_list->ai_protocol); 
  if (socket_fd == -1) {
    perror("socket()");
    exit(EXIT_FAILURE);
  }

  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  if (status == -1){
    perror("setsockopt()");
    exit(EXIT_FAILURE);
  }

  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    perror("bind()");
    exit(EXIT_FAILURE);
  }

  status = listen(socket_fd, num_players);
  if (status == -1) {
    perror("listen()");
    exit(EXIT_FAILURE);
  } 
  
  //printf("Waiting for player connections on port %s\n", port_num);

  player_info players[num_players];
  player_setup player_setups[num_players];

  for (int i = 0; i < num_players; i++){

    memset(&players[i], 0, sizeof(players[i]));

    players[i].player_id = i;
    players[i].is_connected = 0;

    struct sockaddr_storage socket_addr;
    players[i].address_len = sizeof(socket_addr);

    players[i].socket = accept(socket_fd,
			       &players[i].player_address,
			       &players[i].address_len);

    if (players[i].socket == -1){
      perror("accept()");
      exit(EXIT_FAILURE);
    }
    players[i].is_connected = 1;
    
    response r;
    ssize_t size = recv(players[i].socket, &r, sizeof(r),0);
    if (size == -1){
      perror("recv()");
      exit(EXIT_FAILURE);
    }
    snprintf(players[i].port, 6, "%s", r.port_num);

    //printf("Player %d has connected\n",i);
    //print_address_sock(&players[i].player_address);
    //printf("port %s\n", players[i].port);
  }
  
  // players listen/accept left, connect right (need right player's listening port for setup)
  for (int i = 0; i < num_players ; i++){
    
    memset(&player_setups[i], 0, sizeof(player_setups[i]));

    player_setups[i].player_id = i;
    player_setups[i].total_players = num_players;
    player_setups[i].player_addr = players[i].player_address;
    
    if (i == 0){
      player_setups[i].left_id = num_players - 1;   
      player_setups[i].left_addr = players[num_players - 1].player_address;
    }
    else{
      player_setups[i].left_id = i - 1;
      player_setups[i].left_addr = players[i - 1].player_address;
    }
    if (i == (num_players - 1)){
      player_setups[i].right_id = 0;
      player_setups[i].right_addr = players[0].player_address;
      snprintf(player_setups[i].right_port, 6, "%s", players[0].port);   
    }
    else{
      player_setups[i].right_id = i + 1;
      player_setups[i].right_addr = players[i + 1].player_address;
      snprintf(player_setups[i].right_port, 6, "%s", players[i + 1].port); 
      // port right neighbor listens on ^
    }

    players[i].player_setup = &player_setups[i];

    // send setup info to player
    ssize_t status = send(players[i].socket, &player_setups[i], 
			  sizeof(player_setups[i]), 0);
    if (status == -1){
      perror("send() setup info");
      exit(EXIT_FAILURE);
    }

    response player_ready;
    status = recv(players[i].socket, &player_ready, sizeof(player_ready), MSG_WAITALL);
    if (status == -1){ 
      perror("recv() player ack");
      exit(EXIT_FAILURE);
    }

    if (player_ready.player_id == player_setups[i].player_id){
      printf("Player %d is ready to play\n", players[i].player_id);

      //printf("Player %d is listening on port %s and connected to right player port %s\n",
      //players[i].player_id, 
      //player_ready.port_num, player_setups[i].right_port);
    }
    else{
      fprintf(stderr,"Communication error, player sent %d, expected %d\n",
	      player_ready.player_id, players[i].player_id);
      exit(EXIT_FAILURE);
    }
  }
  
  // players have been setup, now start game 

  srand(time(NULL));

  int start_player = rand() % num_players;
  
  printf("Ready to start the game, sending potato to player %d\n", start_player);
  
  // buffer for potato
  potato buffer[2];
  for (int i = 0; i < 2; i++){
    memset(&buffer[i], 0 , sizeof(buffer[i]));
  }
  buffer[0] = hot_potato;
  
  // now monitor the set of file discriptors for the end of the game 
  fd_set player_fds_set;
  FD_ZERO(&player_fds_set);
  
  for (int i = 0; i < num_players; i++){
    FD_SET(players[i].socket, &player_fds_set);
  }

  //printf("Starting game\n");
  send(players[start_player].socket, buffer, sizeof(buffer), 0);

  while(1){
    
    // Block until input arrives from a socket signaling end of game (check read)
    if (select((players[num_players -1].socket + 1), &player_fds_set, NULL, NULL, NULL) == -1){
      // nfds parameter ^ set to highest FD + 1 (according to man pages)
      perror("select()");
      exit(EXIT_FAILURE);
    }
    
    // receive data from FD with pending input 
    for (int i = 0; i < num_players; i++){
      if (FD_ISSET(players[i].socket, &player_fds_set)){ // game has ended

	ssize_t bytes = recv(players[i].socket, buffer, sizeof(buffer), MSG_WAITALL);

	if (bytes == -1){
	  perror("Error receiving potato at game end");
	  exit(EXIT_FAILURE);
	}
	//printf("Received ending potato transmission of %li bytes\n", bytes);
	//printf("Game end triggered at player %d\n", i);	
	
	// send shutdown message
	int check;
	for (int i = 0; i < num_players; i++){
	  check = shutdown(players[i].socket, SHUT_RDWR);
	  if (check != 0){
	    fprintf(stderr,"Shutdown call on player %d\n", i);
	  }
	}

	hot_potato = buffer[0];

	printf("Trace of potato:\n");
	
	if (num_hops == 0){ // then just do one print
	  printf("%d\n", hot_potato.player_ids[0]);
	}
	else{
	  for (int i = 0; i < num_hops; i++){
	    
	    if (i == (num_hops - 1)){ // last printout, no comma
	      printf("%d\n", hot_potato.player_ids[i]);
	    }
	    else{
	      printf("%d, ", hot_potato.player_ids[i]);
	    }
	  }
	}

	freeaddrinfo(host_info_list);  
	close(socket_fd);
	return EXIT_SUCCESS;
      }
    }

    FD_ZERO(&player_fds_set);  // reset fds for select() call
    for (int i = 0; i < num_players; i++){
      FD_SET(players[i].socket, &player_fds_set);
    }

  }
  fprintf(stderr,"Process ended outside of game loop\n");

  return EXIT_SUCCESS;
}

  
