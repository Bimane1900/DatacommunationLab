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

  //size we want to send, the header + lenght of data field.
  int size = (sizeof(struct header)) + packet.head.length2;
  /*int randomCorrupt = rand()%100;
  if(randomCorrupt < 30){
	  packet.head.crc = 2;
  }*/
  //packet.head.crc = 2;

  //serialize the packet to be able to pass it to sendto as buffer
  buffer = serialize_UDP (packet);
  nOfBytes = sendto(fileDescriptor, buffer, size,0,(struct sockaddr*) &receiver, sizeof(struct sockaddr));
  free(buffer);
  //free(testPkt.data);
  if(nOfBytes < 0) {
    perror("writeMessage - Could not write data\n");
    exit(EXIT_FAILURE);
  }
}

/* readMessageFrom
 * Reads and prints data read from the file (socket
 * denoted by the file descriptor 'fileDescriptor'.
 */
int readMessageFrom(int fileDescriptor, rtp* packet, struct sockaddr_in6* address) {
	struct sockaddr_in6 sender;
  char* buffer;
  //prepare buffer for MAXMSG, we do not know how big packet will be
  buffer = calloc(MAXMSG, 1);
  int nOfBytes;
  socklen_t size = sizeof(sender);
  nOfBytes = recvfrom(fileDescriptor, buffer, MAXMSG,0,(struct sockaddr*)&sender,&size);
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
	  //printf("in function.c address? %s\n",sender.sin6_addr.s6_addr);
      *packet = deserialize_UDP (buffer);
	  *address = sender;
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
rtp prepareSYNpkt(int winSize, int seq){
  rtp SYNpkt;
  SYNpkt.data = calloc(messageLength,1);
  SYNpkt.data = "";
  SYNpkt.head.seq = seq;
  SYNpkt.head.windowsize = winSize;
  SYNpkt.head.flags = SYN;
  SYNpkt.head.id = 0;
  SYNpkt.head.length2 = strlen(SYNpkt.data)+1;
  SYNpkt.head.timestamp = time(NULL);
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
  SYN_ACKpkt.head.timestamp = time(NULL);
  return SYN_ACKpkt;
}

/*Prepares ACK on packet with sequence
 * number seq*/
rtp prepareACK(int seq){
  rtp ACKpkt;
  ACKpkt.data = calloc(messageLength,1);
  ACKpkt.data = "";
  ACKpkt.head.seq = seq;
  ACKpkt.head.windowsize = 0;
  ACKpkt.head.flags = ACK;
  ACKpkt.head.id = 0;
  ACKpkt.head.length2 = strlen(ACKpkt.data)+1;
  ACKpkt.head.timestamp = time(NULL);
  return ACKpkt;
}

/*Prepares NACK on expected packet*/
rtp prepareNACK(int expectedPktSeq){
  rtp NACKpkt;
  NACKpkt.data = calloc(messageLength,1);
  NACKpkt.data = "";
  NACKpkt.head.seq = expectedPktSeq;
  NACKpkt.head.windowsize = 0;
  NACKpkt.head.flags = NACK;
  NACKpkt.head.id = 0;
  NACKpkt.head.length2 = strlen(NACKpkt.data)+1;
  NACKpkt.head.timestamp = time(NULL);
  return NACKpkt;
}

/*prepares a packet to send with seq num set to
 * sendingSeq and data is msgToSend*/
rtp preparePKT(char* msgToSend, int sendingSeq){
  rtp newPKT;
  newPKT.data= calloc((strlen(msgToSend)+1),1);
  strncpy(newPKT.data,msgToSend,(strlen(msgToSend)+1));
  newPKT.head.seq = sendingSeq;
  newPKT.head.windowsize = 0;
  newPKT.head.flags = 0;
  newPKT.head.id = 0;
  newPKT.head.length2 = strlen(newPKT.data)+1;
  newPKT.head.timestamp = time(NULL);
  return newPKT;
}

/*prepares a FIN packet*/
rtp prepareFIN(int sendingSeq){
  rtp FINpkt;
  FINpkt.data = calloc(messageLength,1);
  FINpkt.data = "";
  FINpkt.head.seq = sendingSeq;
  FINpkt.head.windowsize = 0;
  FINpkt.head.flags = FIN;
  FINpkt.head.id = 0;
  FINpkt.head.length2 = strlen(FINpkt.data)+1;
  FINpkt.head.timestamp = time(NULL);
  return FINpkt;
}

rtp prepareFIN_ACK(int sendingSeq){
  rtp FINACK;
  FINACK.data = calloc(messageLength,1);
  FINACK.data = "";
  FINACK.head.seq = sendingSeq;
  FINACK.head.windowsize = 0;
  FINACK.head.flags = FIN_ACK;
  FINACK.head.id = 0;
  FINACK.head.length2 = strlen(FINACK.data)+1;
  FINACK.head.timestamp = time(NULL);
  return FINACK;
}

//Checks if packet is valid and a SYN_ACK
// returns 1 if true, else 0
int recievedSYN_ACK(rtp packet){
  if(packet.head.flags == SYN_ACK && packet.head.seq != INVALID_SEQ){
    return 1;
  }
  return 0;
};

//Checks if packet is valid and a SYN
// returns 1 if true, else 0
int recievedSYN(rtp packet){
  if(packet.head.flags == SYN && packet.head.seq != INVALID_SEQ){
    return 1;
  }
  return 0;
}

/*checks if packet is valid, is used
 * when finding new packets. returns 1
 * when packet is valid which means
 * we recieved a packet*/
int receivedPKT(rtp packet){
  if(packet.head.seq != -1){
    return 1;
  }else{
    return 0;
  }
}

/*Returns 1 if packet is a FIN*/
int recievedFIN(rtp packet){
	if(packet.head.flags == FIN){
		return 1;
	}
	return 0;
}

/*Returns 1 if packet is a FIN_ACK*/
int recievedFIN_ACK(rtp packet){
	if(packet.head.flags == FIN_ACK){
		return 1;
	}
	return 0;
}

/*Checks if we recieved expected packet by
 * comparing the sequence numbers*/
int expectedPKT(rtp packet, int expectedPkt){
  if(packet.head.seq == expectedPkt){
    return 1;
  }
  return 0;
}

/*Checks if we recived and expected ACK*/
int expectedACK(rtp packet, int expectedAck){
  if(packet.head.seq == expectedAck && packet.head.flags == ACK){
    return 1;
  }
  return 0;
}

/*Checks if packet is an acceptable packet to recieve
 * by comparing it will list of accepting sequence num*/
int acceptablePKT(rtp packet, int acceptList[(ServWinSize)-1]){
  for(int i = 0; i < (ServWinSize)-1 ; i++){
    if(packet.head.seq == acceptList[i]){
      return 1;
    }
  }
  return 0;
}

/*Check if packet has acceptable sequence number
 * and if it has flags ACK*/
int acceptableACK(rtp packet, int acceptList[(ServWinSize)-1]){
  for(int i = 0; i < (ServWinSize)-1 ; i++){
    if(packet.head.seq == acceptList[i] && packet.head.flags == ACK){
      return 1;
    }
  }
  return 0;
}

/*Check if packet is a valid NACK, will be if sequence number
 * is acceptable or expected*/
int validNACK(rtp packet, int acceptList[(ServWinSize)-1], int expected){
	if(packet.head.seq == expected && packet.head.flags == NACK){
		return 1;
	}
  for(int i = 0; i < (ServWinSize)-1 ; i++){
    if(packet.head.seq == acceptList[i] && packet.head.flags == NACK){
      return 1;
    }
  }
  return 0;
}

/*initializes buff by setting sequence
 * numbers of all indexes to -1
 * (-1 is the "empty" slot value)*/
void initBuffer(rtp buff[BUFFSIZE]){
  for (int i =0 ; i < BUFFSIZE; i++)
    {
      buff[i].head.seq = INVALID_SEQ;
    }
}

/*inserts a packet to buff, if there was no
 * slot (no sequence num was -1) then it
 * returns 0 without inserting. returns 1
 * if succesful*/
int push(rtp buff[BUFFSIZE], rtp packet){
  for (int i = 0; i < BUFFSIZE ; i++)
    {
      if(buff[i].head.seq == INVALID_SEQ){
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
  if(seq == INVALID_SEQ){
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

  rtp error; error.head.seq = INVALID_SEQ;
  return error;
}

/*returns any new PKT recieved in buff
 * if not new PKT exists, returns invalid
 * PKT*/
rtp findNewPKT(rtp buff[BUFFSIZE]){
  for (int i = 0; i < BUFFSIZE; i++)
    {
      if(buff[i].head.seq != INVALID_SEQ){
        return buff[i];
      }
    }
  rtp error;
  error.head.seq = INVALID_SEQ;
  return error;
}

/*removes packet from buffer by
 * setting its seq to -1, making
 * it invalid*/
void pop(rtp buff[BUFFSIZE], int seq){
  for (int i =0 ; i < BUFFSIZE; i++)
    {
      if(buff[i].head.seq == seq){
        buff[i].head.seq = INVALID_SEQ;
        return;
      }
    }
}

/*prints out all packets in buff*/
void printBuff(rtp buff[BUFFSIZE]){
  for(int i = 0; i<BUFFSIZE; i++){
    printf("packet %d: seq %d flags %d\n",i,buff[i].head.seq,buff[i].head.flags);
  }
}

/*Updates sequence number that is used to
 * send next packet. Will se to starting seq
 * if reached a limit*/
void updateSendSeq(int* sendSeq,int winsize, int startSeq){
  (*sendSeq)++;
  if((*sendSeq) == winsize*2 + startSeq){
    (*sendSeq) = startSeq;
  }
}

/*updates which sequence numbers are currently
 * acceptable to recieve, sets to startin seq
 * if reached limit*/
void updateAcceptablePKTs(int acceptablePkts[], int size, int slideWin, int startSeq){
  for(int i = 0; i < size ; i++){
    acceptablePkts[i]++;
    if(acceptablePkts[i] == slideWin*2 + startSeq)
      acceptablePkts[i] = startSeq;
  }
}

/*Slides window by updating expected packet seq
 * and updating acceptable packet seq
 * *expectedPKT - sequece num of expected PKT
 * acceptList - list of sequence number that is accepted
 * slideWin - window size
 * startSeq - first sequence number used*/
void slideWindow(int *expectedPKT, int acceptList[],int slideWin, int startSeq){
  (*expectedPKT)++;
  if((*expectedPKT) == slideWin*2 + startSeq)
    (*expectedPKT) = startSeq;
  updateAcceptablePKTs(acceptList,(slideWin)-1, slideWin, startSeq);
}

/*Resends any packets in buff where timeout is triggered*/
void resendTimeouts(rtp buff[BUFFSIZE],int sock, struct sockaddr_in serverName){
  for(int i = 0; i < BUFFSIZE; i++){
    if(time(NULL) -  buff[i].head.timestamp > TIMEOUT && buff[i].head.seq != -1){
      printf("Resending %s, seq: %d\n",buff[i].data,buff[i].head.seq);
      writeMessage (sock, buff[i], serverName);
      buff[i].head.timestamp = time(NULL);
    }
  }
}

/*Calculates if window is full or not
 * sendSeq - sequence number of next packet to send
 * expected - what sequence number we expect to slide window
 * windowsize - windowsize to compare to*/
int windowIsFull(int sendSeq, int expected, int windowsize){
printf("sendSeq: %d, expected %d, windowsize %d\n", sendSeq, expected,windowsize);
  int diff;
  if(sendSeq >= expected){
    diff = sendSeq - expected;
  //  printf("diff: %d\n",diff);
    if(diff >= (windowsize))
      return 1;
    else return 0;
  }
  else{
    diff = expected - sendSeq;
    //printf("diff: %d\n",diff);
    if(diff <= (windowsize))
      return 1;
    else return 0;
  }
}





