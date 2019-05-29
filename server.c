/*
 * Simon Roysson and Mareks Ozols
 * 
 * server.c - contains state machines for
 * communication with client. Will recieve packets
 * and respond with ACK. Type server in terminal
 * to allow client.c to connect and start
 * communications*/
#include "header.h"

  rtp packet; // global packet that state machine can use as temp. (maybe can be in main instead)
  int timer = 0; //timer for timeouts (maybe can be in main too)
  rtp sendPackets[BUFFSIZE]; //buffer for storing sent packets incase resend is needed, remove packet when ACK is received
  rtp recvPackets[BUFFSIZE]; //buffer for storing recieved packets, remove packet when sending ACK on a packet
  rtp acceptedPackets[BUFFSIZE]; //buffer for storing acceptable packets, we check this one everytime expected packet is updated

/*readMessages
 * function that is used by thread which checks if there has
 * come in some message from client by calling
 * function readMessageFrom
 * */
void* readMessages(void* socket){
  int* sock = (int*)socket;
  rtp packet,temp;
  struct sockaddr_in6 address;
  while(1){
    if(readMessageFrom(*sock, &packet, &address)){
      //Check the crc field, if it does not match, dont process packet
      if(packet.head.crc == Checksum(packet)){
		temp = findPacket(recvPackets, packet.head.seq, -1);
		//Check if packet with that sequence already exists, push only if not
		if(temp.head.seq == -1){
        //if buffer is not full, push it to recieverbuffer so state machine can read it
			if(!push(recvPackets,packet)){
			  printf("Buffer full, packet thrown away\n");
			}
		}else{
			printf("Sequence number already in packet\n");
		}
      }else{
        printf("Checksum corrupt: seq : %d\n",packet.head.seq);
      }
    }
  }
}

int main(int argc, char *argv[]) {
  int sock;
  int serverState = CONN_SETUP;
  int subState = WAIT_SYN;
  struct sockaddr_in clientName;
  pthread_t listener;
  int expectedPkt; //Which packet to expect, is set in connection
  int acceptablePkts[ServWinSize-1]; //acceptable packets sequence numbers
  rtp NACKpkt; //variable to keep track of NACK
  NACKpkt.head.seq = INVALID_SEQ;
  int recivingWindow = ServWinSize; //Set to servers allowed winSize
  int startSeq; //what seq will sender start with, used to set max sequence number to start

  //initiate the buffers to get correct comparisions
  initBuffer(recvPackets);
  initBuffer(sendPackets);
  initBuffer(acceptedPackets);


  /* Create a socket and set it up to listen */
  sock = makeSocket(PORT);

  /*Hardcoded client address*/
  initSocketAddress (&clientName,"127.0.0.1",PORT+1);
  
  srand(time(NULL));//random generator for corrupting packets

  pthread_create(&listener, NULL, readMessages,(void*)&sock);
  /*Listen for messages and reply*/
  while(1){
      switch (serverState){
    case CONN_SETUP:

        switch (subState){
        case WAIT_SYN:
          /*use findPacket to lookout for packets
           * with the SYN-flag*/
          packet = findPacket (recvPackets,-1,SYN);
          if(recievedSYN(packet)){
            printf("recieved SYN, seq: %d\n",packet.head.seq);
            //prepare a SYN_ACK with same sequence number as SYN packet
            packet = prepareSYN_ACK (packet.head.seq);
			packet.head.windowsize = recivingWindow; //set server windowsize for client
            startSeq = packet.head.seq; //client want to send starting with this sequence num
            expectedPkt = packet.head.seq; //so we expect it 
			//initiate acceptable packets
			for(int i = 0; i < ServWinSize-1; i++){
				acceptablePkts[i] = expectedPkt+i+1;
			}
            subState = SET_CHECKSUM;
          }
         break;

        case SET_CHECKSUM:
          //set checksum and send SYN_ACK
          if(packet.head.flags == SYN_ACK){
            packet.head.crc = Checksum(packet); //set Checksum for SYN_ACK
            writeMessage (sock,packet,clientName); //send SYN_ACK packet
            pop(recvPackets,packet.head.seq); //remove recieved SYN from buffer
            printf("Sent SYN_ACK, seq: %d\n",packet.head.seq);
            timer = time(NULL); //set timer for timeout
            subState = WAIT_SYN_ACK;
          }
          break;

        case WAIT_SYN_ACK:
          /*Wait some time before going into established
		   * incase something went wrong*/
          if(time(NULL) - timer > TIMEOUT+TIMEOUT){
			  subState = EST_CONN;
			  printf("Timer\n");
              printf("EST_CONN\n");
          }
		  /*if recieve another SYN, then some packets
		   * have not been recieved properly*/
		  packet = findPacket(recvPackets,-1,SYN);
		  if(recievedSYN(packet)){
			  subState = WAIT_SYN;
		  }
          /* Tries to find packet with SYN_ACK flag
           * in recv packet. if -statement will be true when
           * found which means we got the SYN_ACK*/
          packet = findPacket (recvPackets,-1,SYN_ACK);
          if(recievedSYN_ACK (packet)){
              printf("Recieved SYN_ACK, seq: %d\n",packet.head.seq);
              pop(recvPackets,packet.head.seq);//pop SYN_ACK
              subState = EST_CONN;
            printf("EST_CONN\n");
          }
          break;

        case EST_CONN:
          serverState = SLIDING_WINDOW;
          subState = WAIT_PKT;
          printf("expected: %d \n\n",expectedPkt);
          break;

        default:
          break;
        }
      break;
    case SLIDING_WINDOW:

      switch (subState){
      case WAIT_PKT:
		/*We wait for new packets if state will become true
		 * when recieved and we change state*/
		 //First try to find expected packet
		packet = findPacket(recvPackets,expectedPkt,-1);
		if(expectedPKT(packet,expectedPkt)){
			printf("\n");
			printf("Recieved packet with data: %s\n",packet.data);
			subState = READ_SEQ_NUMBER;
		}
		//If expected packet no found, process any packet in buffer
        packet = findNewPKT(recvPackets);
        if(receivedPKT(packet)){
		  if(recievedSYN(packet)){ //client did not recieve final SYN_ACK, send a new
			  prepareSYN_ACK(packet.head.seq); 
			  packet.head.crc = Checksum(packet);
			  writeMessage(sock,packet,clientName);//send a new SYN_ACK
			  printf("Sending SYN_ACK, seq %d\n",packet.head.seq);
			  pop(recvPackets,packet.head.seq);//pop SYN from buffer when SYN_ACK sent
			  break;
		  }
		  if(recievedFIN(packet)){ // received FIN packet so we trigger Teardown and change states
			  subState = WAIT_FIN;
			  serverState = CONN_TEARDOWN;
		  }else{ // nothing above was received, look at the sequence num to find what it is
			  printf("\n");
			  printf("Recieved packet with data: %s\n",packet.data);
			  subState = READ_SEQ_NUMBER;
		  }
        }
          break;
      case READ_SEQ_NUMBER:
        printf("new packet seq: %d , expectedpkt: %d\n",packet.head.seq,expectedPkt);
		//Expected packet was recieved so prepare an ACK
        if(expectedPKT(packet, expectedPkt)){
          printf("Preparing ACK\n");
          packet = prepareACK(packet.head.seq); //prep ACK
          subState = SET_CHECKSUM;
          break;
        }
		//Recieved packet have acceptable sequence num
		//save the packet and prepare ACK and NACK
        if(acceptablePKT(packet, acceptablePkts)){
          printf("Preparing ACK and NACK\n");
		  push(acceptedPackets,packet); //save packets in accepted buff
          packet = prepareACK(packet.head.seq); //prep ACK
          NACKpkt = prepareNACK(expectedPkt); //prep NACK
          subState = SET_CHECKSUM;
          break;
        }
		/*Reach this point if packet had
		 * unaccpetable sequence number*/
		packet = prepareACK(packet.head.seq);
		packet.head.crc = Checksum(packet);
		writeMessage(sock,packet,clientName);//send a new ACK but dont do anything else
        pop(recvPackets,packet.head.seq);
        subState = WAIT_PKT;
          break;
      case SET_CHECKSUM:
	    /*If NACKpkt is invalid, then we know we
		 * recieved expected and are sending ACK and not NACK*/
        if(NACKpkt.head.seq == INVALID_SEQ){
          packet.head.crc = Checksum(packet);
          printf("Sent ACK %d\n",packet.head.seq);
          writeMessage (sock, packet, clientName); //send ACK
          pop(recvPackets,packet.head.seq);	//Packet can be popped since sent ACK
		  //Can slide window because packet was the expected packet, acceptable and expected are updated
          slideWindow(&expectedPkt, acceptablePkts,recivingWindow,startSeq); 
          subState = READ_BUFFER;
        }else{
		  /*When recieved accepted packet, we send ACK on it
		   * but also NACK on expected packet*/
          packet.head.crc = Checksum(packet);
          NACKpkt.head.crc = Checksum(NACKpkt);
          printf("Sent ACK %d and NACK %d\n",packet.head.seq,NACKpkt.head.seq);
          writeMessage (sock,packet,clientName);//send ACK
          pop(recvPackets,packet.head.seq); //Packet can be popped since sent ACK
          writeMessage (sock,NACKpkt,clientName);//send NACK
          NACKpkt.head.seq = INVALID_SEQ; //Reset NACK for future comparisons
          subState = WAIT_PKT;
        }
          break;
      case READ_BUFFER:
	    /*Try to read expected packet from accepted buffer*/
        packet = findPacket (acceptedPackets,expectedPkt,-1);
		/*If expected packet is in buffer then we can process it
		 * and slide window again*/
        if(expectedPKT(packet,expectedPkt)){
          //processPKT
          printf("Data already in buffer: %s\n", packet.data);
          slideWindow(&expectedPkt, acceptablePkts,recivingWindow,startSeq); //Can slide window again
          pop(acceptedPackets,packet.head.seq); //pop packet from accepted, we processed it
        }else{
          printf("Expected not in buffer\n");
          subState = WAIT_PKT;
        }
          break;
      default:
          break;
      }
      break;
    case CONN_TEARDOWN:

      switch (subState){

        case WAIT_FIN:
		  //Start here when received a FIN packet
		  //will prepare FIN_ACK
		  if(recievedFIN(packet)){
			  printf("WAIT_FIN: recieved FIN\n");
			  pop(recvPackets,packet.head.seq);
			  packet = prepareFIN_ACK(packet.head.seq);
			  subState = SET_CHECKSUM;
		  }else{
			 subState = WAIT_PKT; 
			 serverState = SLIDING_WINDOW;
		  }
          break;

        case SET_CHECKSUM:
			//Sending FIN_ACK and FIN
			packet.head.crc = Checksum(packet);
			writeMessage(sock,packet,clientName);
			//pop(recvPackets,packet.head.seq);//Can pop recieved FIN
			printf("Send FIN_ACK\n");
			packet = prepareFIN(0);
			packet.head.crc = Checksum(packet);
			writeMessage(sock,packet,clientName);
			printf("Send FIN\n");
			push(sendPackets,packet);//store FIN incase of resend
			subState = WAIT_FIN_ACK;
          break;

        case WAIT_FIN_ACK:
			resendTimeouts(sendPackets,sock,clientName);//resend FIN if timeout
			//Wait until we receive FIN_ACK and then closes
			packet = findPacket(recvPackets,-1,FIN_ACK);
			if(recievedFIN_ACK(packet)){
				printf("Recieved FIN_ACK\n");
				pop(sendPackets,packet.head.seq);//pop FIN_ACK
				subState = CLOSED;
				printf("Closed!\n");
				break;
			}
			packet = findPacket(recvPackets,-1,FIN);
			if(recievedFIN(packet)){ 
				//client still resending FIN, the sent FIN_ACK  lost
				//pop resending FIN and redo from WAIT_FIN
				printf("Recieved FIN\n");
				subState = WAIT_FIN;
				pop(sendPackets,0);
			}
          break;

        case CLOSED:
			//remove client from list of connected clients
          break;

        default:
          break;
        }
      break;


    }
  }
}

