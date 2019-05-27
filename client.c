/* File: client.c
 * Trying out socket communication between processes using the Internet protocol family.
 * Usage: client [host name], that is, if a server is running on 'lab1-6.idt.mdh.se'
 * then type 'client lab1-6.idt.mdh.se' and follow the on-screen instructions.
 */
#include "header.h"


  rtp packet; // global packet that state machine can use as temp. (maybe can be in main instead)
  int timer = 0; //timer for timeouts (maybe can be in main too)
  rtp sendPackets[BUFFSIZE]; //buffer for storing sent packets incase resend is needed, remove packet when ACK is received
  rtp recvPackets[BUFFSIZE]; //buffer for storing recieved packets, remove packet when sending ACK on a packet
  rtp acceptedPackets[BUFFSIZE];
  int ConnRequest = 1; //hardcoded trigger right now to get the connection to start
  char msgToSend[messageLength] = "";



/*
 * Input loop used on client to be able to send messages*/
void* readInput (void* socketInfo){
  char messageString[messageLength];
  //argument* sockInfo = (argument*) socketInfo;    //TODO better name on argument TYPE
  /*printf("\nType something and press [RETURN] to send it to the server.\n");
  printf("Type 'quit' to nuke this program.\n");*/
  fflush(stdin);
  while(1) {
    printf("\n>");
    fgets(messageString, messageLength, stdin);
    messageString[strlen(messageString) - 1] = '\0';
    if(strncmp(messageString,"quit\n",messageLength) != 0){
      strncpy(msgToSend,messageString,messageLength);
    }
    else {
      //close(sockInfo->fileDescriptor);
      //exit(EXIT_SUCCESS);
    }
  }
}


/*readServerMessage
 * function that is used by thred which checs if there has
 * come in som message from server by calling
 * function readMessageFromServer
 * */
void* readServerMessage (void* socket){
  int* sock = (int*)socket;
  rtp packet,temp;
  struct sockaddr_in6 address;
  while(1){
    if(readMessageFrom(*sock, &packet, &address)){
		//printf("adress? %s\n", address.sin6_addr.s6_addr);
      //Check the crc field, if it does not match, dont process packet
      if(packet.head.crc == Checksum(packet)){
        temp =findPacket (recvPackets,packet.head.seq,-1);
		//Check if packet with that sequence already exists, push only if not
        if(temp.head.seq == -1){
        //if buffer is not full, push it to recieverbuffer so state machine can read it
          if(!push(recvPackets,packet)){
            printf("Buffer full, packet thrown away\n");
          }
        }else{
		  //printBuff(recvPackets);
          printf("packet with seqNum %d already in buffer\n",packet.head.seq);
        }
      }else{
        printf("Checksum corrupt\n");
      }
    }
  }
}



int main(int argc, char *argv[]) {
  int sock;
  struct sockaddr_in serverName;
  char hostName[hostNameLength];
  pthread_t writeToServer;
  pthread_t readFromServer;
  int test1;
  int test2;
  int senderState = CONN_SETUP;
  int machineState = INIT;
  rtp send_buffer;
  argument input;
  int expectedAck; //expected ack sequence number, set when first packet is sent
  int acceptableAcks[ClientWinSize/2+1]; //keeps sequence number for acceptable acks/nacks
  acceptableAcks[0] = 11;
  acceptableAcks[1] = 12;
  int sendingSeq = 10; // current sending sequence number
  int startingSeq = 10; //save starting sequence 
  int sendingWindow = ClientWinSize; //clients requested winsize
  
  int randomDrop = 0;
  int randomcorrupt = 0;

  //initiate the buffers to get correct comparisions
  initBuffer(recvPackets);
  initBuffer(sendPackets);
  initBuffer(acceptedPackets);


  /*hardcoded IP for easy testing*/
  strncpy(hostName, "127.0.0.1",hostNameLength);

  /* Create the socket, client need to listen to another port
   * because it is on same machine I think, else server will
      pick up its own messages */
  sock = makeSocket(PORT+1);

  /* Initialize the socket address */
  initSocketAddress(&serverName, hostName, PORT);


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
	  
    while(1){

    switch (senderState){
    case CONN_SETUP:
        switch (machineState){
        case INIT:
          if(ConnRequest == 1){
            /*start by preparing a SYN*/
            packet = prepareSYNpkt (sendingWindow, startingSeq);
            machineState = SET_CHECKSUM;
            //TODO should set clients prefered winsize and sequence number to start
          }
          break;

        case SET_CHECKSUM:
          /*If current packet we are sending is a SYN
           * then we set a timer and update our expectedAck
           * to be ready for SYN_ACK and be able to resend*/
          if(packet.head.flags == SYN){
            packet.head.crc = Checksum(packet); //set Checksum
            writeMessage (sock, packet, serverName); //send SYN
            printf("send SYN, seq: %d\n",packet.head.seq);
            expectedAck = packet.head.seq; //Update expected to be same seq number
            push(sendPackets,packet); //push packet to sendBuffer incase we need to resend
            timer = time(NULL); //set timer
            machineState = WAIT_SYN_ACK;
          }
          /*If current packet is a SYN_ACK, we dont
           * have to set timer and such, there is no
           * expectation of resending either*/
          if(packet.head.flags == SYN_ACK){
            packet.head.crc = Checksum(packet);
            writeMessage (sock, packet, serverName);
            pop(recvPackets,packet.head.seq);
            printf("send SYN_ACK, seq: %d\n",packet.head.seq);
            machineState = EST_CONN;
            printf("EST_CONN!\n");
          }
          break;

        case WAIT_SYN_ACK:
          resendTimeouts (sendPackets, sock,serverName); //Check for timeouts
          /*Use findpacket to try get expectedAck
           * the if-statement will become true when we
           * have the correct ACK*/
          packet = findPacket (recvPackets,expectedAck,-1);
          if(recievedSYN_ACK(packet)){
             printf("recieved SYN_ACK, seq: %d\n",packet.head.seq);
             pop(sendPackets,expectedAck); //Can pop the resend packet now because we recieved ack
             pop(recvPackets,packet.head.seq); //pop the ACK
             slideWindow (&expectedAck,acceptableAcks,sendingWindow,startingSeq); //update because we expected next sequence number
             machineState = WAIT_SYN;
          }
          break;

        case WAIT_SYN:
          /*use findPacket to try get packets with
           * SYN flags on them*/
          packet = findPacket (recvPackets,-1,SYN);
          if(recievedSYN(packet)){
            printf("recieved SYN, seq: %d\n",packet.head.seq);
            sendingWindow = packet.head.windowsize;
            packet = prepareSYN_ACK (packet.head.seq);
            machineState = SET_CHECKSUM;
          }
          break;
        case EST_CONN:
          senderState = SLIDING_WINDOW;
          machineState = WINDOW_NOT_FULL;
          updateSendSeq(&sendingSeq,sendingWindow,startingSeq);
          break;

        default:
          break;
        }
      break;
    case SLIDING_WINDOW:

      switch (machineState){

        case WINDOW_NOT_FULL:
          resendTimeouts(sendPackets, sock, serverName); //Check for timeouts
          packet = findNewPKT(recvPackets);
		  //If recieved packet, check sequence number
          if(receivedPKT (packet)){
			  printf("\n");
            printf("Recieved packet, seq: %d\n",packet.head.seq);
            machineState = READ_SEQ_NUMBER;
          }
		  //If msgToSend is updated, will send data if it is
		  else if(strcmp(msgToSend,"")!= 0){
			  if(strcmp(msgToSend,"disconnect") == 0){
				  machineState =INIT;
				  senderState = CONN_TEARDOWN;
			  }
			  else{
				printf("\n");
				packet = preparePKT(msgToSend, sendingSeq); //prepare packet with msgToSend as data
				strncpy(msgToSend,"",messageLength);
				machineState = SET_CHECKSUM;
			  } 
          }
          break;
		  
        case WINDOW_FULL:
          resendTimeouts(sendPackets, sock, serverName); //Check for timeouts
          packet = findNewPKT(recvPackets);
		  //If recieved packet, check sequence number
          if(receivedPKT (packet)){
            printf("Recieved packet, seq: %d\n",packet.head.seq);
            machineState = READ_SEQ_NUMBER;
          }
          break;
		  
        case SET_CHECKSUM:
          packet.head.crc = Checksum(packet);
          randomcorrupt = rand()%100; //Random corrupt packet by manipulating crc (fixed seed for rand)
          if(randomcorrupt > 50){
            packet.head.crc = 2;
            writeMessage (sock, packet, serverName); //send corrupted packet
            packet.head.crc = Checksum(packet); //next resend will be good packet
          }else{
            writeMessage (sock, packet, serverName); //send good packet
          }
          printf("Sent packet, seq: %d\n",packet.head.seq);
          updateSendSeq(&sendingSeq,sendingWindow, startingSeq); //update out next sending sequence number
          push(sendPackets, packet); //push packet to sendPackets for timeouts and resending
          machineState = CHECK_WINDOW_SIZE;
          break;
		  
        case CHECK_WINDOW_SIZE:
		/*go to different states depending if window size is full*/
          if(windowIsFull(sendingSeq,expectedAck,sendingWindow)){
            printf("WINDOW FULL\n");
            machineState = WINDOW_FULL;
          }else{
            printf("Window is not full\n");
            machineState = WINDOW_NOT_FULL;
          }
          break;
		  
        case READ_SEQ_NUMBER:
		printf("expeckted ack: %d\n",expectedAck);
		  //if packet is expected ACK, then process it and slide window
          if(expectedACK(packet, expectedAck)){
            printf("Recieved expectec ACK, seq: %d\n",packet.head.seq);
            pop(recvPackets, expectedAck); //pop from recvPackets
            pop(sendPackets, expectedAck); //pop from sendPacket to stop resending
            slideWindow(&expectedAck,acceptableAcks, sendingWindow,startingSeq);
            machineState = READ_BUFFER;
          }
		  //if packet is acceptable ACK then we store it in accepte buffer
		  else if(acceptableACK(packet,acceptableAcks)){
            printf("Recieved acceptable ACK, seq: %d\n",packet.head.seq);
            pop(sendPackets,packet.head.seq); //pop from sendpackets to stop resending
			pop(recvPackets,packet.head.seq); //pop from recvPackets
			push(acceptedPackets, packet); //push to acceptedPackets for when client looks if expected packts is there
            machineState = CHECK_WINDOW_SIZE;
          }
		  //if packet is valid NACK, then try to resend packet which NACK references
		  else if(validNACK(packet,acceptableAcks,expectedAck)){
            printf("Recieved acceptable NACK, seq: %d\n",packet.head.seq);
            packet = findPacket (sendPackets,packet.head.seq, -1);
			if(packet.head.seq != -1){			
				pop(sendPackets, packet.head.seq); //pop to update packet
				packet.head.timestamp = time(NULL); //update timestamp for resending
				packet.head.crc = Checksum(packet); //update checksum (hmm maybe dont need to, timestamp not used in Checksum
				push(sendPackets,packet); //push updated packet back to sendPackets
				printf("Resending packet with seq: %d\n", packet.head.seq);
				writeMessage (sock,packet,serverName); //resend packet
				pop(recvPackets, packet.head.seq); //pop NACK
			}
			machineState = CHECK_WINDOW_SIZE;
          }else{
			//unaccpetable packet, pop it !
            printf("Recieved unacceptable packet seq : %d\n", packet.head.seq);
            pop(recvPackets, packet.head.seq);
            machineState = CHECK_WINDOW_SIZE;
          }
          break;
		  
        case READ_BUFFER:
          packet = findPacket (acceptedPackets, expectedAck, -1);
		  //if expected ACK is in acceptedPackets, we can process it and pop it and also slideWindow more
          if(expectedACK(packet,expectedAck)){
			  //Process ACK
             printf("Expectd ack was in buffer, seq: %d\n",packet.head.seq);
			 pop(acceptedPackets,packet.head.seq); 
             slideWindow(&expectedAck,acceptableAcks, sendingWindow,startingSeq);
          }else{
             machineState = CHECK_WINDOW_SIZE;
          }
          break;
      }
      break;
    case CONN_TEARDOWN:

      switch (machineState){

        case INIT:
			printf("init disconnect\n");
			packet = prepareFIN(sendingSeq);
			machineState = SET_CHECKSUM;
          break;

        case SET_CHECKSUM:
			packet.head.crc = Checksum(packet);
			writeMessage(sock,packet,serverName);
			if(packet.head.flags == FIN_ACK){
				printf("FIN_ACK sent\n");
				timer = time(NULL);
				machineState = WAIT;
			}
			else{
				printf("FIN sent\n");
				push(sendPackets, packet);
				machineState = WAIT_FIN_ACK;
			}
          break;

        case WAIT_FIN_ACK:
			resendTimeouts(sendPackets,sock,serverName);
			packet = findPacket(recvPackets,-1,FIN_ACK);
			if(recievedFIN_ACK(packet)){
				printf("FIN_ACK recieved\n");
				pop(recvPackets,packet.head.seq);
				machineState = WAIT_FIN;
			}
          break;

        case WAIT_FIN:
			packet = findPacket(recvPackets, -1, FIN);
			if(recievedFIN(packet)){
				printf("FIN recieved\n");
				packet = prepareFIN_ACK(packet.head.seq);
				machineState = SET_CHECKSUM;
			}
          break;
		  
        case WAIT:
			if(time(NULL)-timer >= 10){
				machineState = CLOSED;
			}
          break;
		  
        case CLOSED:
			printf("Closing socket\n");
			close(sock);
			exit(EXIT_SUCCESS);
          break;

        default:
          break;
        }
      break;


    }
  }

  pthread_join(writeToServer, NULL);
  pthread_join(readFromServer, NULL);

}
