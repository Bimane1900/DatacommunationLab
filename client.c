/* File: client.c
 * Trying out socket communication between processes using the Internet protocol family.
 * Usage: client [host name], that is, if a server is running on 'lab1-6.idt.mdh.se'
 * then type 'client lab1-6.idt.mdh.se' and follow the on-screen instructions.
 */
#include "header.h"

int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in serverName;
  char hostName[hostNameLength];
  pthread_t writeToServer;
  pthread_t readFromServer;
  int test1;
  int test2;
  int senderState;
  int machineState;
  rtp send_buffer;
  argument input;



  /*while(1){

    switch (senderState){
    case CONN_SETUP:

        switch (machineState){

        case INIT:
          break;

        case SET_CHECKSUM:
          break;

        case WAIT_SYN_ACK:
          break;

        case READ_CHECKSUM:
          break;

        case EST_CONN:
          senderState = 1;
          break;

        default:
          break;
        }
      break;
    case SLIDING_WINDOW:

      switch (machineState){

        case WINDOW_NOT_FULL:
          break;
        case WINDOW_FULL:
          break;
        case SET_CHECKSUM:
          break;
        case CHECK_WINDOW_SIZE:
          break;
        case READ_CHECKSUM:
          break;
        case READ_SEQ_NUMBER:
          break;
        case READ_BUFFER:
          break;
          break;
      }
      break;
    case CONN_TEARDOWN:

      switch (machineState){

        case INIT:
          break;

        case SET_CHECKSUM:
          break;

        case WAIT_FIN_ACK:
          break;

        case READ_CHECKSUM:
          break;

        case WAIT_FIN:
          senderState = 1;
          break;
        case WAIT:
          break;
        case CLOSED:
          break;

        default:
          break;
        }
      break;


    }
  }*/



  /* Check arguments */
  /*if(argv[1] == NULL) {
    perror("Usage: client [host name]\n");
    exit(EXIT_FAILURE);
  }
  else {
    strncpy(hostName, argv[1], hostNameLength);
    hostName[hostNameLength - 1] = '\0';
  }*/

  /*hardcoded IP for easy testing*/
  strncpy(hostName, "127.0.0.1",hostNameLength);

  /* Create the socket, client need to listen to another port
   * because it is on same machine I think, else server will
      pick up its own messages */
  sock = makeSocket(PORT+1);

  /*sock = socket(PF_INET, SOCK_DGRAM, 0);
  if(sock < 0) {
    perror("Could not create a socket\n");
    exit(EXIT_FAILURE);
  }*/

  /* Initialize the socket address */
  initSocketAddress(&serverName, hostName, PORT);

  /* Connect to the server */
  /*if(connect(sock, (struct sockaddr *)&serverName, sizeof(serverName)) < 0) {
    perror("Could not connect to server\n");
    exit(EXIT_FAILURE);
  }
  else{*/

    input.fileDescriptor = sock;
    input.server = serverName;


    /*Create thread that will check incoming messages from server and print them on screen*/
    test1 = pthread_create(&readFromServer, NULL,readServerMessage, (void*) &sock);
    if(test1 != 0)
      printf("%d : %s\n",errno,strerror(errno));
    /*Create thread that will wait for input from user and will send it further to server*/
    test2 = pthread_create(&writeToServer,NULL,readInput,(void*)&input);
    if(test2 != 0)
      printf("%d : %s\n", errno,strerror(errno));
  //}
  pthread_join(writeToServer, NULL);
  pthread_join(readFromServer, NULL);
}
