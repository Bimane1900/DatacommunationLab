/* 
 * Simon Roysson and Mareks Ozols
 * 
 * client.c - contains state machines for client-side
 * communication with server.c. Can send packets with
 * precoded array and userInput. Type client in terminal 
 * to connect to running server on same machine (loopback)
 */
 
#include "header.h"


  rtp packet; // global packet that state machine can use as temp. (maybe can be in main instead)
  int timer = 0; //timer for timeouts (maybe can be in main too)
  rtp sendPackets[BUFFSIZE]; //buffer for storing sent packets incase resend is needed, remove packet when ACK is received
  rtp recvPackets[BUFFSIZE]; //buffer for storing recieved packets, remove packet when sending ACK on a packet
  rtp acceptedPackets[BUFFSIZE]; //buffer for storing accepted packets, will be remove when read as expected
  int ConnRequest = 1; //hardcoded trigger right now to get the connection to start
  char msgToSend[messageLength] = ""; //check with this variable is there is anything to send from user input



/*
 * Input loop used on client to be able to read input into mstToSend variable*/
void* readInput (void* socketInfo){
  char messageString[messageLength];
  fflush(stdin);
  while(1) {
    printf("\n>");
    fgets(messageString, messageLength, stdin);
    messageString[strlen(messageString) - 1] = '\0';
    if(strncmp(messageString,"quit\n",messageLength) != 0){
      strncpy(msgToSend,messageString,messageLength);
    }
  }
}


/*readServerMessage
 * function that is used by thread which checks if there has
 * come in some message from server by calling
 * function readMessageFromServer
 * */
void* readServerMessage (void* socket){
  int* sock = (int*)socket;
  rtp packet,temp;
  struct sockaddr_in6 address;
  while(1){
    if(readMessageFrom(*sock, &packet, &address)){
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
          printf("packet with seqNum %d already in buffer\n",packet.head.seq);
        }
      }else{
        printf("Checksum corrupt seq : %d\n",packet.head.seq);
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
  int reader;
  int userinput;
  int senderState = CONN_SETUP;
  int machineState = INIT;
  int expectedAck; //expected ack sequence number, set when first packet is sent
  int acceptableAcks[ClientWinSize-1]; //keeps sequence number for acceptable acks/nacks
  int sendingSeq = 10; // current sending sequence number
  int startingSeq = 10; //save starting sequence 
  int sendingWindow = ClientWinSize; //clients requested winsize
  int tempPacketSeq;
  rtp isInBuffPacket;
  
  //variable to send packets a lot of packet, so we can see sliding window data flow
  const char *testSending[] = {"a","b","c","d","e","f"};
  int sendIndex = 0;
  int sendTestSize = 6;


  //initiate the buffers to get correct comparisions
  initBuffer(recvPackets);
  initBuffer(sendPackets);
  initBuffer(acceptedPackets);


  /*hardcoded IP for easy testing*/
  strncpy(hostName, "127.0.0.1",hostNameLength);

  /* Create the socket, client need to listen to another port
   * because it is on same machine, else server will
      pick up its own messages */
  sock = makeSocket(PORT+1);

  /* Initialize the socket address */
  initSocketAddress(&serverName, hostName, PORT);

	srand(time(NULL));//new seed for random numbers, used for generating errors in packets

    /*Create thread that will check incoming messages from server and print them on screen*/
    reader = pthread_create(&readFromServer, NULL,readServerMessage, (void*) &sock);
    if(reader != 0)
      printf("%d : %s\n",errno,strerror(errno));
    /*Create thread that will wait for input from user and will send it further to server*/
    userinput = pthread_create(&writeToServer,NULL,readInput,NULL);
    if(userinput != 0)
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
           * then we update our expectedAck to be ready
           * for SYN_ACK and is added to our sendbuffer
		   * be able to resend if SYN is lost*/
          if(packet.head.flags == SYN){
            packet.head.crc = Checksum(packet); //set Checksum
            writeMessage (sock, packet, serverName); //send SYN
            printf("send SYN, seq: %d\n",packet.head.seq);
            expectedAck = packet.head.seq; //Update expected to be same seq number
            push(sendPackets,packet); //push packet to sendBuffer incase we need to resend
            machineState = WAIT_SYN_ACK;
          }
          /*If current packet is a SYN_ACK, 
		   * there is no expectation of 
		   * resending so we dont have to add to sendbuffer*/
          if(packet.head.flags == SYN_ACK){
            packet.head.crc = Checksum(packet);
            writeMessage (sock, packet, serverName); //send SYN_ACK
            printf("send SYN_ACK, seq: %d\n",packet.head.seq);
            machineState = EST_CONN;
			timer = time(NULL); //set timer for EST_CONN state
          }
          break;

        case WAIT_SYN_ACK:
          resendTimeouts (sendPackets, sock,serverName); //Check for timeouts
          /*Use findpacket to try get expectedAck
           * the if-statement will become true when we
           * have the correct ACK*/
          packet = findPacket (recvPackets,expectedAck,-1);//could also probably search for SYN_ACK with third parameter
          if(recievedSYN_ACK(packet)){
             printf("recieved SYN_ACK, seq: %d\n",packet.head.seq);
             pop(sendPackets,expectedAck); //Can pop the resend packet now because we recieved ack
             pop(recvPackets,packet.head.seq); //pop the ACK
			 sendingWindow = packet.head.windowsize; //set sending window to servers windowsize
			 for(int i = 0; i < sendingWindow-1; i++){ //set how many acceptable ACKS we have based on windowsize
				 acceptableAcks[i]= expectedAck+i+1;
			 }
             packet = prepareSYN_ACK (packet.head.seq);
             machineState = SET_CHECKSUM;
          }
          break;

        case EST_CONN:
		  //the timer is there to have some silence before sliding window starts(Testing purposes to see output)
		  if(time(NULL)-timer > TIMEOUT){
			  printf("EST_CONN!\n");
			  //parameters for expected and sequence number to use going into sliding window
			  printf("expected: %d  senderSeq: %d\n",expectedAck, sendingSeq); 
			  printf("\n");
			senderState = SLIDING_WINDOW;
            machineState = WINDOW_NOT_FULL;
		  }
          break;

        default:
          break;
        }
      break;
    case SLIDING_WINDOW:

      switch (machineState){

        case WINDOW_NOT_FULL:
          resendTimeouts(sendPackets, sock, serverName); //Check for timeouts
		  /*Automatic sending of packets will trigger as long as this is true
		   * and it will be for 6 packets as default, can be changed at variables.
		   * Used to get a data flow to evaluate sliding window behavior*/
		  if(sendIndex < sendTestSize){
			  packet = preparePKT(testSending[sendIndex], sendingSeq);
			  machineState = SET_CHECKSUM;
			  sendIndex++;
			  break;
		  }
		  //First we try to process expected ACK if exists in buffer
		  packet = findPacket(recvPackets,expectedAck, -1);
		  if(expectedACK(packet, expectedAck)){
			printf("\n");
            printf("Recieved expected packet, seq: %d\n",packet.head.seq);
            machineState = READ_SEQ_NUMBER;
			break;
		  }
		  //if recieved non-expeceted packet, check sequence number
		  packet = findNewPKT(recvPackets);
          if(receivedPKT (packet)){
			printf("\n");
            printf("Recieved packet, seq: %d\n",packet.head.seq);
            machineState = READ_SEQ_NUMBER;
			break;
          }
		  //If msgToSend isnt empty, will send data if it is
		  //preparing a packet.
		  if(strcmp(msgToSend,"")!= 0){
			  if(strcmp(msgToSend,"disconnect") == 0){ //initate teardown if string is disconnect
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
		  //First we try to process expected ACK if exists in buffer
		  packet = findPacket(recvPackets,expectedAck, -1);
		  if(expectedACK(packet, expectedAck)){
			printf("\n");
            printf("Recieved expected packet, seq: %d\n",packet.head.seq);
            machineState = READ_SEQ_NUMBER;
			break;
		  }
		  //if recieved non-expeceted packet, check sequence number
          packet = findNewPKT(recvPackets);
          if(receivedPKT (packet)){
            printf("Recieved packet, seq: %d\n",packet.head.seq);
            machineState = READ_SEQ_NUMBER;
          }
          break;
		  
        case SET_CHECKSUM:
          packet.head.crc = Checksum(packet); //set checksum on packet before sending
          printf("Sent packet, seq: %d\n",packet.head.seq);
		  writeMessage (sock, packet, serverName); //send packet
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
		  printf("\n");
          break;
		  
        case READ_SEQ_NUMBER:
		printf("expeckted ack: %d\n",expectedAck);
		  //if packet is expected ACK, then process it and slide window
          if(expectedACK(packet, expectedAck)){
            printf("Recieved expectec ACK, seq: %d\n",packet.head.seq);
            pop(recvPackets, expectedAck); //pop from recvPackets
            pop(sendPackets, expectedAck); //pop from sendPacket to stop resending
			//can slide window when expected ACK is processed
            slideWindow(&expectedAck,acceptableAcks, sendingWindow,startingSeq); 
            machineState = READ_BUFFER;
          }
		  //if packet is acceptable ACK then we store it in accepted buffer
		  else if(acceptableACK(packet,acceptableAcks)){
            printf("Recieved acceptable ACK, seq: %d\n",packet.head.seq);
            pop(sendPackets,packet.head.seq); //pop from sendpackets to stop resending
			pop(recvPackets,packet.head.seq); //pop from recvPackets
			isInBuffPacket = findPacket(acceptedPackets,packet.head.seq, -1);
			if(isInBuffPacket.head.seq = INVALID_SEQ){
				push(acceptedPackets, packet); //push to acceptedPackets for when client looks if expected packts is there
			}
			else{
				printf("ACK already in accepted buffer\n"); //Dont want to add same packets to buffer
			}
            machineState = CHECK_WINDOW_SIZE;
          }
		  //if packet is valid NACK, then try to resend packet which NACK references
		  else if(validNACK(packet,acceptableAcks,expectedAck)){
            printf("Recieved acceptable NACK, seq: %d\n",packet.head.seq);
			tempPacketSeq = packet.head.seq;
			//check if we have packet that NACK is referencing with its sequence num
            packet = findPacket (sendPackets,packet.head.seq, -1);
			if(packet.head.seq != INVALID_SEQ){			
				pop(sendPackets, packet.head.seq); //pop to update packet
				packet.head.timestamp = time(NULL); //update timestamp for resending
				packet.head.crc = Checksum(packet); //update checksum (hmm maybe dont need to, timestamp not used in Checksum
				push(sendPackets,packet); //push updated packet back to sendPackets
				printf("Resending packet with seq: %d\n", packet.head.seq);
				writeMessage (sock,packet,serverName); //resend packet
				pop(recvPackets, packet.head.seq); //pop NACK
			}
			//if we dont have packet, we cant respond, pop the NACK
			//The packet has most likely been popped from recieving ACK
			pop(recvPackets,tempPacketSeq);
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
			//start by preparing a FIN packet to send to server
			printf("init disconnect\n");
			packet = prepareFIN(sendingSeq);
			machineState = SET_CHECKSUM;
          break;

        case SET_CHECKSUM:
			/*When sending FIN to let server
			 * know we want to disconnect we store
			 * packet in sendbuffer incase of resend
			 * but when sending FIN_ACK, there is no
			 * expectation of resending*/
			packet.head.crc = Checksum(packet);
			writeMessage(sock,packet,serverName);
			if(packet.head.flags == FIN_ACK){
				printf("FIN_ACK sent\n");
				timer = time(NULL); //Start timer for WAIT-state
				machineState = WAIT;
			}
			else{
				printf("FIN sent\n");
				push(sendPackets, packet); //store FIN incase of resend
				machineState = WAIT_FIN_ACK;
			}
          break;

        case WAIT_FIN_ACK:
			resendTimeouts(sendPackets,sock,serverName); //resend any packets with timeouts
			//Look for FIN_ACKS in buffer
			packet = findPacket(recvPackets,-1,FIN_ACK);
			if(recievedFIN_ACK(packet)){
				printf("FIN_ACK recieved\n");
				pop(recvPackets,packet.head.seq); //can pop FIN_ACK when we processed it
				pop(sendPackets,packet.head.seq); //can pop FIN since we dont need to resend it now(we got its ACK)
				machineState = WAIT_FIN;
			}
          break;

        case WAIT_FIN:
			//Look for FIN packet in buffer to process
			packet = findPacket(recvPackets, -1, FIN);
			if(recievedFIN(packet)){
				printf("FIN recieved\n");
				pop(recvPackets,packet.head.seq);
				packet = prepareFIN_ACK(packet.head.seq); // respond with FIN_ACK
				machineState = SET_CHECKSUM;
			}
          break;
		  
        case WAIT:
			//timer to let server close down safely as in TCP
			if(time(NULL)-timer >= TIMEOUT){
				machineState = CLOSED;
			}
          break;
		  
        case CLOSED:
			//Teardown complete, exiting
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
