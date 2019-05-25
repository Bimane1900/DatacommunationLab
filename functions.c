#include "header.h"

/*
 * serializes a packet into a buffer and returns that buffer*/
char * serialize_UDP(rtp udp){
  //allocate memory for both header and the length of data.
  // Can not do sizeof(rtp) because data field is dynamic.
  char * buffer2 = calloc(udp.head.length2+sizeof(struct header),1);

  memcpy(buffer2, &(udp.head),sizeof(struct header)); //the header
  memcpy((buffer2+sizeof(struct header)),(udp.data), udp.head.length2); //the data

  return buffer2;
  //free(buffer2);

}

/*
 * Deserializes a packet from buffer, returns the packet*/
rtp deserialize_UDP(char* buffer){
  rtp udp;
  //Dont know how long data field is.
  // prepare for messageLength
  udp.data = calloc(messageLength,1);

  memcpy(&(udp.head), buffer, sizeof(struct header)); //the header
  memcpy((udp.data), (buffer+sizeof(struct header)), udp.head.length2); //the data
                                                                    //
  return udp;
  //free(udp.data);
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
void writeMessage(int fileDescriptor, rtp packet, struct sockaddr_in receiver) {
  int nOfBytes;
  char* buffer;
  buffer = calloc(MAXMSG,1);
  //Test packet
  /*testPkt.data = calloc(messageLength,sizeof(char));
  testPkt.data = message;
  testPkt.head.seq = 2;
  testPkt.head.id = 3;
  testPkt.head.windowsize = 4;
  testPkt.head.flags = 5;
  testPkt.head.crc = Checksum (testPkt);
  testPkt.head.length2 = strlen(testPkt.data)+1;*/
  /*printf("start data: %s\n",packet.data);
  printf("start crc: %d\n",packet.head.crc);
  printf("start windowsize: %d\n",packet.head.windowsize);
  printf("start flags: %d\n",packet.head.flags);
  printf("start seq: %d\n",packet.head.seq);
  printf("start id: %d\n\n",packet.head.id);*/

  //size we want to send, the header + lenght of data field.
  int size = (sizeof(struct header)) + packet.head.length2;

  //serialize the packet to be able to pass it to sendto as buffer
  buffer = serialize_UDP (packet);
  nOfBytes = sendto(fileDescriptor, buffer, size,0,(struct sockaddr*) &receiver, sizeof(struct sockaddr));
  //free(buffer);
  //free(testPkt.data);
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
  /*printf("\nType something and press [RETURN] to send it to the server.\n");
  printf("Type 'quit' to nuke this program.\n");*/
  fflush(stdin);
  while(1) {
    //printf("\n>");
    fgets(messageString, messageLength, stdin);
    //remove \n
    messageString[strlen(messageString) - 1] = '\0';
    if(strncmp(messageString,"quit\n",messageLength) != 0){
      //Test packet
      rtp testPkt;
      testPkt.data = calloc(messageLength,sizeof(char));
      testPkt.data = messageString;
      testPkt.head.seq = -2;
      testPkt.head.id = -3;
      testPkt.head.windowsize = -4;
      testPkt.head.flags = -1;
      testPkt.head.crc = Checksum (testPkt);
      testPkt.head.length2 = strlen(testPkt.data)+1;
      writeMessage(sockInfo->fileDescriptor, testPkt,sockInfo->server );

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
int readMessageFrom(int fileDescriptor, rtp* packet) {
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
      *packet = deserialize_UDP (buffer);
      /*printf("data: %s\n",packet->data);
      printf("crc: %d\n",packet->head.crc);
      printf("windowsize: %d\n",packet->head.windowsize);
      printf("flags: %d\n",packet->head.flags);
      printf("seq: %d\n",packet->head.seq);
      printf("id: %d\n\n",packet->head.id);*/
      free(buffer);
      return 1;
    }
  return 0;
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

int Checksum (rtp packet)
{
  int i = 0;
  int checksum;

  checksum = packet.head.flags + packet.head.id + packet.head.seq + packet.head.windowsize + packet.head.length2;

  for(i = 0;/*packet.data[i] != '\0'||*/ i < packet.head.length2;i++)
    {
    checksum += packet.data[i];
    }
  return checksum;
}

/*prepares a SYNpkt and returns it
 * can specify winSize with argument
 * but should be able to specify seq aswell*/
rtp prepareSYNpkt(int winSize){
  rtp SYNpkt;
  SYNpkt.data = calloc(messageLength,1);
  SYNpkt.data = "";
  SYNpkt.head.seq = 0;
  SYNpkt.head.windowsize = winSize;
  SYNpkt.head.flags = SYN;
  SYNpkt.head.id = 0;
  SYNpkt.head.length2 = strlen(SYNpkt.data)+1;
  return SYNpkt;
}

/*prepares a SYN_ACK with a specifed seq num*/
rtp prepareSYN_ACK (int seq){
  rtp SYN_ACKpkt;
  SYN_ACKpkt.data = calloc(messageLength,1);
  SYN_ACKpkt.data = "";
  SYN_ACKpkt.head.seq = seq;
  SYN_ACKpkt.head.windowsize = 0;
  SYN_ACKpkt.head.flags = SYN_ACK;
  SYN_ACKpkt.head.id = 0;
  SYN_ACKpkt.head.length2 = strlen(SYN_ACKpkt.data)+1;
  return SYN_ACKpkt;
}

//Checks if packet is valid and a SYN_ACK
// returns 1 if true, else 0
int recievedSYN_ACK(rtp packet){
  if(packet.head.flags == SYN_ACK && packet.head.seq != -1){
    return 1;
  }
  return 0;
};

//Checks if packet is valid and a SYN
// returns 1 if true, else 0
int recievedSYN(rtp packet){
  if(packet.head.flags == SYN && packet.head.seq != -1){
    return 1;
  }
  return 0;
}

/*initializes buff by setting sequence
 * numbers of all indexes to -1
 * (-1 is the "empty" slot value)*/
void initBuffer(rtp buff[BUFFSIZE]){
  for (int i =0 ; i < BUFFSIZE; i++)
    {
      buff[i].head.seq = -1;
    }
}

/*inserts a packet to buff, if there was no
 * slot (no sequence num was -1) then it
 * returns 0 without inserting. returns 1
 * if succesful*/
int push(rtp buff[BUFFSIZE], rtp packet){
  for (int i = 0; i < BUFFSIZE ; i++)
    {
      if(buff[i].head.seq == -1){
        buff[i] = packet;
        return 1;
      }
    }
  return 0;
}

/* Tries to find a packet.
 * can find by looking at sequence numbers
 * or by looking at flags. Returns the packet.
 * If not found, returns an invalid packet*/
rtp findPacket(rtp buff[BUFFSIZE], int seq, int flags){
  if(seq == -1){
    for (int i = 0; i < BUFFSIZE; i++)
      {
        if(buff[i].head.flags == flags){
          return buff[i];
        }
      }
  }
  if(flags == -1){for (int i = 0; i < BUFFSIZE; i++)
    {
      if(buff[i].head.seq == seq){
        return buff[i];
      }
    }
  }

  rtp error; error.head.seq = -1;
  return error;
}

/*removes packet from buffer by
 * setting its seq to -1, making
 * it invalid*/
void pop(rtp buff[BUFFSIZE], int seq){
  for (int i =0 ; i < BUFFSIZE; i++)
    {
      if(buff[i].head.seq == seq){
        buff[i].head.seq = -1;
      }
    }
}


