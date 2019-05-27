
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
		//printf("address? %s\n",address.sin6_addr.s6_addr);
      if(packet.head.crc == Checksum(packet)){
		temp = findPacket(recvPackets, packet.head.seq, -1);
		if(temp.head.seq == -1){
			if(!push(recvPackets,packet)){
			  printf("Buffer full, packet thrown away\n");
			}
		}else{
			printf("Sequence number already in packet\n");
		}
      }else{
        printf("Checksum corrupt: %d is not %d\n",packet.head.crc,Checksum(packet));
      }
    }
  }
}

int main(int argc, char *argv[]) {
  int sock;
  int serverState = CONN_SETUP;
  int subState = WAIT_SYN;
  struct sockaddr_in clientName;
  socklen_t size;
  pthread_t listener;
  int senderSeq = 0;
  int expectedPkt; //Which packet to expect, is set in connection
  int acceptablePkts[ServWinSize/2-1]; //acceptable packets sequence numbers
  acceptablePkts[0] = 11;//hardcoded, Could also be set in connection
  acceptablePkts[1] = 12;
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
            //prepare a SYN_ACK with same sequence number as SYNpacket
            packet = prepareSYN_ACK (packet.head.seq);
            startSeq = packet.head.seq;
            expectedPkt = packet.head.seq;
            subState = SET_CHECKSUM;
          }
         break;

        case SET_CHECKSUM:
          //set checksums and send SYN_ACK and SYN
          if(packet.head.flags == SYN_ACK){
            packet.head.crc = Checksum(packet); //set Checksum for SYN_ACK
            writeMessage (sock,packet,clientName); //send SYN_ACK packet
            pop(recvPackets,packet.head.seq); //remove recieved SYN from buffer
            printf("Sent SYN_ACK, seq: %d\n",packet.head.seq);
            packet = prepareSYNpkt (recivingWindow,senderSeq); //prepare SYN
            //packet.head.seq = expectedPkt; // maybe can do this in prepare function
            packet.head.crc = Checksum(packet); //set Checksum for SYN
            writeMessage (sock,packet,clientName); //send SYN
            printf("Send SYN, seq: %d\n",packet.head.seq);
            push(sendPackets,packet); //push SYN to buffer incase we need to resend
            timer = time(NULL); //set timer for timeout
            subState = WAIT_SYN_ACK;
          }
          break;

        case WAIT_SYN_ACK:
          /*Checks for timeouts, resend packet if triggered*/
          if(time(NULL) - timer > 10){
            printf("resend SYN\n");
            //can find packet to resend with findPacket by sequence num
            writeMessage(sock,findPacket(sendPackets,senderSeq,-1),clientName);
            timer = time(NULL);
          }
          /* Tries to find packet with expected sequence number
           * in recv packet. if -statement will be true when
           * found which means we got the SYN_ACK*/
          packet = findPacket (recvPackets,-1,SYN_ACK);
          if(recievedSYN_ACK (packet)){
              printf("Recieved SYN_ACK, seq: %d\n",packet.head.seq);
              pop(sendPackets,packet.head.seq);//can remove the resending packet from sendbuffer(we got the ack on it)
              pop(recvPackets,packet.head.seq);//pop SYN_ACK
              slideWindow (&expectedPkt,acceptablePkts,recivingWindow,startSeq);
              subState = EST_CONN;
            printf("EST_CONN\n");
          }
          break;

        case EST_CONN:
          serverState = SLIDING_WINDOW;
          subState = WAIT_PKT;
          printf("expected: %d  senderSeq: %d\n",expectedPkt, senderSeq);
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
        packet = findNewPKT(recvPackets);
        if(receivedPKT(packet)){
		  if(recievedFIN(packet)){
			  subState = WAIT_FIN;
			  serverState = CONN_TEARDOWN;
		  }else{
			  printf("\n");
			  printf("Recieved packet with data: %s\n",packet.data);
			  subState = READ_SEQ_NUMBER;
		  }
        }
          break;
      case READ_SEQ_NUMBER:
        //printf("new packet seq: %d , expectedpkt: %d\n",packet.head.seq,expectedPkt);
        //printf("acceptable packets %d %d\n",acceptablePkts[0],acceptablePkts[1]);
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
		  push(acceptedPackets,packet); //save packets in acceptedBuff
          packet = prepareACK(packet.head.seq); //prep ACK
          NACKpkt = prepareNACK(expectedPkt); //prep NACK
          subState = SET_CHECKSUM;
          break;
        }
		/*Reach this point if packet had
		 * unaccpetable sequence number*/
        pop(recvPackets,packet.head.seq);
        subState = WAIT_PKT;
          break;
      case SET_CHECKSUM:
	    /*If NACKpkt is invalid, then we know we
		 * recieved expected and are sending ACK but not NACK*/
        if(NACKpkt.head.seq == INVALID_SEQ){
          packet.head.crc = Checksum(packet);
          printf("Sent ACK %d\n",packet.head.seq);
          writeMessage (sock, packet, clientName); //send ACK
          pop(recvPackets,packet.head.seq);	//Packet can be popped since sent ACK
          slideWindow(&expectedPkt, acceptablePkts,recivingWindow,startSeq); //Can slide window because packet was the expected packet, acceptable and expected are updated
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
          NACKpkt.head.seq = -1; //Reset NACK for future comparisons
          subState = WAIT_PKT;
        }
          break;
      case READ_BUFFER:
	  /*Try to read expected packet from accepted buffer*/
        packet = findPacket (acceptedPackets,expectedPkt,-1);
		printBuff(acceptedPackets);
		/*If expected packet is in buffer then we can process it
		 * and slide window again*/
        if(expectedPKT(packet,expectedPkt)){
          //processPKT
          printf("Data already in buffer: %s\n", packet.data);
          slideWindow(&expectedPkt, acceptablePkts,recivingWindow,startSeq); //Can slide window again
          pop(acceptedPackets,packet.head.seq); //pop packet frmo accepted, we processed it
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
		  if(recievedFIN(packet)){
			  printf("WAIT_FIN: recieved FIN\n");
			  packet = prepareFIN_ACK(packet.head.seq);
			  subState = SET_CHECKSUM;
		  }else{
			 subState = WAIT_PKT; 
			 serverState = SLIDING_WINDOW;
		  }
          break;

        case SET_CHECKSUM:
			packet.head.crc = Checksum(packet);
			writeMessage(sock,packet,clientName);
			printf("Send FIN_ACK\n");
			packet = prepareFIN(0);
			packet.head.crc = Checksum(packet);
			writeMessage(sock,packet,clientName);
			printf("Send FIN\n");
			push(sendPackets,packet);
			subState = WAIT_FIN_ACK;
          break;

        case WAIT_FIN_ACK:
			packet = findPacket(recvPackets,-1,FIN_ACK);
			if(recievedFIN_ACK(packet)){
				printf("Recieved FIN_ACK\n");
				pop(sendPackets,packet.head.seq);
				subState = CLOSED;
				printf("Closed!\n");
			}
			resendTimeouts(sendPackets,sock,clientName);
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

