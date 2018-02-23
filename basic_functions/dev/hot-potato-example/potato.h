#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

/* ECE650 -- Homework3 -- Connor Grehlinger
 * 
 * "Potato" struct for passing and record keeping in Hot Potato game
 */


/* There is a constraint on the number of hops */
#define MAX_HOPS 512 

typedef struct potato_t{
  int num_hops; 
  int player_ids[MAX_HOPS];
  int current_hop; 
  
} potato;


typedef struct player_setup_t{
  int player_id;
  int right_id;
  int left_id;
  struct sockaddr player_addr;
  struct sockaddr left_addr;
  struct sockaddr right_addr;
  int total_players;
  char right_port[6];
} player_setup;


typedef struct player_response{
  int player_id;
  char port_num[6];  // have range 51015 to 51097
} response;



