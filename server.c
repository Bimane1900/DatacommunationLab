
#include "header.h"

  rtp packet; // global packet that state machine can use as temp. (maybe can be in main instead)
  int timer = 0; //timer for timeouts (maybe can be in main too)
  rtp sendPackets[BUFFSIZE]; //buffer for storing sent packets incase resend is needed, remove packet when ACK is received
  rtp recvPackets[BUFFSIZE]; //buffer for storing recieved packets, remove packet when sending ACK on a packet

/*readMessages
 * function that is used by thread which checks if there has
 * come in some message from client by calling
 * function readMessageFrom
 * */
void* readMessages(void* socket){
  int* sock = (int*)socket;
  rtp packet;
  while(1){
    if(readMessageFrom(*sock, &packet)){
      if(packet.head.crc == Checksum(packet)){
        if(!push(recvPackets,packet)){
          printf("Buffer full, packet thrown away\n");
        }
      }else{
        printf("Checksum corrupt\n");
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
  int expectedPkt = 0;
  int acceptablePkts[ClientWinSize/2+1];

  //initiate the buffers to get correct comparisions
  initBuffer(recvPackets);
  initBuffer(sendPackets);



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
            printf("recieved SYN\n");
            //prepare a SYN_ACK with same sequence number as SYNpacket
            packet = prepareSYN_ACK (packet.head.seq);
            subState = SET_CHECKSUM;
          }
         break;

        case SET_CHECKSUM:
          //set checksums and send SYN_ACK and SYN
          if(packet.head.flags == SYN_ACK){
            packet.head.crc = Checksum(packet); //set Checksum for SYN_ACK
            writeMessage (sock,packet,clientName); //send SYN_ACK packet
            pop(recvPackets,packet.head.seq); //remove recieved SYN from buffer
            printf("Sent SYN_ACK\n");
            packet = prepareSYNpkt (ServWinSize); //prepare SYN
            packet.head.seq = expectedPkt; // maybe can do this in prepare function
            packet.head.crc = Checksum(packet); //set Checksum for SYN
            writeMessage (sock,packet,clientName); //send SYN
            printf("Send SYN\n");
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
            writeMessage(sock,findPacket(sendPackets,expectedPkt,-1),clientName);
            timer = time(NULL);
          }
          /* Tries to find packet with expected sequence number
           * in recv packet. if -statement will be true when
           * found which means we got the SYN_ACK*/
          packet = findPacket (recvPackets,expectedPkt,-1);
          if(recievedSYN_ACK (packet)){
              printf("Recieved SYN_ACK\n");
              pop(sendPackets,expectedPkt);//can not remove the resending packet from sendbuffer(we got the ack on it)
              subState = EST_CONN;
            printf("EST_CONN\n");
          }
          break;

        case EST_CONN:
          break;

        default:
          break;
        }
      break;
    case SLIDING_WINDOW:

      switch (subState){

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

      switch (subState){

        case INIT:
          break;

        case SET_CHECKSUM:
          break;

        case WAIT_FIN_ACK:
          break;

        case READ_CHECKSUM:
          break;

        case WAIT_FIN:
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

     /* if(packet.head.crc == Checksum(packet)){
        if(packet.head.flags == SYN){
          printf("received SYN");
          packet.head.flags = SYN_ACK;
          packet.head.crc = Checksum(packet);
          writeMessage (sock,packet,clientName);
        }
      }
    }
      //writeMessage(sock,"I hear you...",clientName);
  }*/

}

