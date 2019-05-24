#include "header.h"

/*
 * serializes a packet into a buffer and returns that buffer*/
char * serialize_UDP(rtp udp){
  //allocate memory for both header and the length of data.
  // Can not do sizeof(rtp) because data field is dynamic.
  char * buffer2 = calloc(udp.head.crc+sizeof(struct header),1);

  memcpy(buffer2, &(udp.head),sizeof(struct header)); //the header
  memcpy((buffer2+sizeof(struct header)),(udp.data), udp.head.crc); //the data

  return buffer2;

}

/*
 * Deserializes a packet from buffer, returns the packet*/
rtp deserialize_UDP(char* buffer){
  rtp udp;
  //Dont know how long data field is.
  // prepare for messageLength
  udp.data = calloc(messageLength,1);

  memcpy(&(udp.head), buffer, sizeof(struct header)); //the header
  memcpy((udp.data), (buffer+sizeof(struct header)), udp.head.crc); //the data
                                                                    //
  return udp;
}

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
 * send packet to a socket.
 */
void writeMessage(int fileDescriptor, char *message, struct sockaddr_in receiver) {
  int nOfBytes;
  char* buffer;
  buffer = calloc(MAXMSG,1);
  //Test packet
  rtp testPkt;
  testPkt.data = calloc(messageLength,sizeof(char));
  testPkt.data = message;
  testPkt.head.seq = 2;
  testPkt.head.id = 3;
  testPkt.head.windowsize = 4;
  testPkt.head.flags = 5;
  testPkt.head.crc = setChecksum (testPkt); //strlen(testPkt.data)+1;
  printf("start data: %s\n",testPkt.data);
  printf("start crc: %d\n",testPkt.head.crc);
  printf("start windowsize: %d\n",testPkt.head.windowsize);
  printf("start flags: %d\n",testPkt.head.flags);
  printf("start seq: %d\n",testPkt.head.seq);
  printf("start id: %d\n\n",testPkt.head.id);

  //size we want to send, the header + lenght of data field.
  int size = (sizeof(struct header)) + testPkt.head.crc;

  //serialize the packet to be able to pass it to sendto as buffer
  buffer = serialize_UDP (testPkt);
  nOfBytes = sendto(fileDescriptor, buffer, size,0,(struct sockaddr*) &receiver, sizeof(struct sockaddr));
  if(nOfBytes < 0) {
    perror("writeMessage - Could not write data\n");
    exit(EXIT_FAILURE);
  }
}

/*
 * Input loop used on client to be able to send messages*/
void* readInput (void* socketInfo){
  char messageString[messageLength];
  argument* sockInfo = (argument*) socketInfo;    //TODO better name on argument TYPE
  printf("\nType something and press [RETURN] to send it to the server.\n");
  printf("Type 'quit' to nuke this program.\n");
  fflush(stdin);
  while(1) {
    printf("\n>");
    fgets(messageString, messageLength, stdin);
    //remove \n
    messageString[strlen(messageString) - 1] = '\0';
    if(strncmp(messageString,"quit\n",messageLength) != 0){

      writeMessage(sockInfo->fileDescriptor, messageString,sockInfo->server );

    }
    else {
      close(sockInfo->fileDescriptor);
      exit(EXIT_SUCCESS);
    }
  }
}
/* readMessageFromServer
 * Reads and prints data read from the file (socket
 * denoted by the file descriptor 'fileDescriptor'.
 */
int readMessageFrom(int fileDescriptor) {
  char* buffer;
  //prepare buffer for MAXMSG, we do not know how big packet will be
  buffer = calloc(MAXMSG, 1);
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
      //test packet
      rtp testPkt;
      //deserialize to put the data from buffer to struct
      testPkt = deserialize_UDP (buffer);
      printf("data: %s\n",testPkt.data);
      printf("crc: %d\n",testPkt.head.crc);
      printf("windowsize: %d\n",testPkt.head.windowsize);
      printf("flags: %d\n",testPkt.head.flags);
      printf("seq: %d\n",testPkt.head.seq);
      printf("id: %d\n\n",testPkt.head.id);
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

int setChecksum (rtp packet)
{
  int i = 0;
  int checksum;

  checksum = packet.head.flags + packet.head.id + packet.head.seq + packet.head.windowsize;

  for(i;packet.data[i] != '\0'|| i < 255;i++)
    {
    checksum += packet.data[i];
    }
  return checksum;
}
