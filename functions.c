#include "header.h"

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
int readMessageFrom(int fileDescriptor) {
  char buffer[MAXMSG];
  int nOfBytes;

  nOfBytes = recvfrom(fileDescriptor, buffer, MAXMSG,0,NULL,NULL);
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
      return 1;
    }
  return(0);
}
/*readServerMessage
 * function that is used by thred which checs if there has
 * come in som message from server by calling
 * function readMessageFromServer
 * */
void* readServerMessage (void* socket){
  int* sock = (int*)socket;
  while(1){
    readMessageFrom(*sock);
  }
}

/*Makes a new socket that will be bind to argument port*/
int makeSocket(unsigned short int port) {
  int sock;
  struct sockaddr_in name;

  /* Create a socket. */
  sock = socket(PF_INET, SOCK_DGRAM, 0);
  if(sock < 0) {
    perror("Could not create a socket\n");
    exit(EXIT_FAILURE);
  }
  /* Give the socket a name. */
  /* Socket address format set to AF_INET for Internet use. */
  name.sin_family = AF_INET;
  /* Set port number. The function htons converts from host byte order to network byte order.*/
  name.sin_port = htons(port);
  /* Set the Internet address of the host the function is called from. */
  /* The function htonl converts INADDR_ANY from host byte order to network byte order. */
  /* (htonl does the same thing as htons but the former converts a long integer whereas
   * htons converts a short.)
   */
  name.sin_addr.s_addr = htonl(INADDR_ANY);
  /* Assign an address to the socket by calling bind. */
  if(bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
    perror("Could not bind a name to the socket\n");
    exit(EXIT_FAILURE);
  }
  return(sock);
}

