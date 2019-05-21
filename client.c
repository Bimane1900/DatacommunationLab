/* File: client.c
 * Trying out socket communication between processes using the Internet protocol family.
 * Usage: client [host name], that is, if a server is running on 'lab1-6.idt.mdh.se'
 * then type 'client lab1-6.idt.mdh.se' and follow the on-screen instructions.
 */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>

#define PORT 5555
#define hostNameLength 50
#define messageLength  256
#define MAXMSG 512

#define SYN 1
#define SYN_ACK 2
#define ACK 4
#define FIN 8
#define FIN_ACK 16
#define NACK 32

#define CONN_SETUP 0
#define SLIDING_WINDOW 1
#define CONN_TEARDOWN 2

#define INIT 0
#define SET_CHECKSUM 1
#define WAIT_SYN_ACK 2
#define READ_CHECKSUM 3
#define EST_CONN 4
#define WAIT_FIN_ACK 5
#define WAIT_FIN 6
#define WAIT 7
#define CLOSED 8
#define WINDOW_NOT_FULL 9
#define CHECK_WINDOW_SIZE 10
#define WINDOW_FULL 11
#define READ_SEQ_NUMBER 12
#define READ_BUFFER 13


/* initSocketAddress
 * Initialises a sockaddr_in struct given a host name and a port.
 */
void initSocketAddress(struct sockaddr_in *name, char *hostName, unsigned short int port) {
  struct hostent *hostInfo; /* Contains info about the host */
  /* Socket address format set to AF_INET for Internet use. */
  name->sin_family = AF_INET;
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name->sin_port = htons(port);
  /* Get info about host. */
  hostInfo = gethostbyname(hostName);
  if(hostInfo == NULL) {
    fprintf(stderr, "initSocketAddress - Unknown host %s\n",hostName);
    exit(EXIT_FAILURE);
  }
  /* Fill in the host name into the sockaddr_in struct. */
  name->sin_addr = *(struct in_addr *)hostInfo->h_addr;
}
/* writeMessage
 * Writes the string message to the file (socket)
 * denoted by fileDescriptor.
 */
void writeMessage(int fileDescriptor, char *message, struct sockaddr_in serverName) {
  int nOfBytes;

  //nOfBytes = write(fileDescriptor, message, strlen(message) + 1);
  nOfBytes = sendto(fileDescriptor, message, strlen(message)+ 1,0,(struct sockaddr*) &serverName, sizeof(struct sockaddr));
  if(nOfBytes < 0) {
    perror("writeMessage - Could not write data\n");
    exit(EXIT_FAILURE);
  }

}
typedef struct threadArgument{
  int fileDescriptor;
  struct sockaddr_in server;
}argument;
/*readInput
 * function that is used by thread which checks if there has
 * come in some input from user if so sends it to function writeMessage
 * */

void* readInput (void* input){
  char messageString[messageLength];
  argument* input2 = (argument*) input;
  printf("\nType something and press [RETURN] to send it to the server.\n");
  printf("Type 'quit' to nuke this program.\n");
  fflush(stdin);
  while(1) {
    printf("\n>");
    fgets(messageString, messageLength, stdin);
    messageString[messageLength - 1] = '\0';
    if(strncmp(messageString,"quit\n",messageLength) != 0){

      writeMessage(input2->fileDescriptor, messageString,input2->server );

    }
    else {
      close(input2->fileDescriptor);
      exit(EXIT_SUCCESS);
    }
  }
}
/* readMessageFromServer
 * Reads and prints data read from the file (socket
 * denoted by the file descriptor 'fileDescriptor'.
 */
int readMessageFromServer(int fileDescriptor) {
  char buffer[MAXMSG];
  int nOfBytes;

  nOfBytes = read(fileDescriptor, buffer, MAXMSG);
  if(nOfBytes < 0) {
    perror("Could not read data from client\n");
    exit(EXIT_FAILURE);
  }
  else
    if(nOfBytes == 0)
      /* End of file */
      return(-1);
    else{
      /* Data read */
      printf("Incoming message: %s\n>",  buffer);
    }
  return(0);
}
/*readServerMessage
 * function that is used by thred which checs if there has
 * come in som message from server by calling
 * function readMessageFromServer
 * */
void* readServerMessage (void* fileDescriptor){
  while(1){
    readMessageFromServer((int)fileDescriptor);
  }
}

typedef struct rtp_struct{
  int flags;
  int id;
  int seq;
  int windowsize;
  int crc;
  char *data;
}rtp;



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



  while(1){

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
  }



  /* Check arguments */
  if(argv[1] == NULL) {
    perror("Usage: client [host name]\n");
    exit(EXIT_FAILURE);
  }
  else {
    strncpy(hostName, argv[1], hostNameLength);
    hostName[hostNameLength - 1] = '\0';
  }
  /* Create the socket */
  sock = socket(PF_INET, SOCK_STREAM, 0);
  if(sock < 0) {
    perror("Could not create a socket\n");
    exit(EXIT_FAILURE);
  }
  /* Initialize the socket address */
  initSocketAddress(&serverName, hostName, PORT);
  /* Connect to the server */
  if(connect(sock, (struct sockaddr *)&serverName, sizeof(serverName)) < 0) {
    perror("Could not connect to server\n");
    exit(EXIT_FAILURE);
  }
  else{

    input.fileDescriptor = sock;
    input.server = serverName;


    /*Create thread that will check incoming messages from server and print them on screen*/
    test1 = pthread_create(&readFromServer, NULL,readServerMessage, (void*) &input);
    if(test1 != 0)
      printf("%d : %s\n",errno,strerror(errno));
    /*Create thread that will wait for input from user and will send it further to server*/
    test2 = pthread_create(&writeToServer,NULL,readInput,(void*)(size_t) sock);
    if(test2 != 0)
      printf("%d : %s\n", errno,strerror(errno));
  }
  //pthread_join(writeToServer, NULL);
  //pthread_join(readFromServer, NULL);
}
