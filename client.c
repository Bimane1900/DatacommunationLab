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
  int ConnRequest = 1; //hardcoded trigger right now to get the connection to start




/*readServerMessage
 * function that is used by thred which checs if there has
 * come in som message from server by calling
 * function readMessageFromServer
 * */
void* readServerMessage (void* socket){
  int* sock = (int*)socket;
  rtp packet;
  while(1){
    if(readMessageFrom(*sock, &packet)){
      //Check the crc field, if it does not match, dont process packet
      if(packet.head.crc == Checksum(packet)){
        //if buffer is not full, push it to recieverbuffer so state machine can read it
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
  int expectedAck = 0; //hardcoded for now
  int acceptableAcks[ClientWinSize/2+1]; //for future use maybe

  //initiate the buffers to get correct comparisions
  initBuffer(recvPackets);
  initBuffer(sendPackets);


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
            packet = prepareSYNpkt (ClientWinSize);
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
            printf("send SYN\n");
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
            printf("send SYN_ACK\n");
            machineState = EST_CONN;
            printf("EST_CONN!\n");
          }
          break;

        case WAIT_SYN_ACK:
          //Current timer is 10 seconds
          if(time(NULL)-timer > 10){
            printf("resend SYN\n");
            //resend packet by using find by sequence number in the send buff
            writeMessage(sock,findPacket(sendPackets,expectedAck,-1),serverName);
            timer = time(NULL); //reset timer
          }
          /*Use findpacket to try get expectedAck
           * the if-statement will become true when we
           * have the correct ACK*/
          packet = findPacket (recvPackets,expectedAck,-1);
          if(recievedSYN_ACK(packet)){
             printf("recieved SYN_ACK\n");
             pop(sendPackets,expectedAck); //Can pop the resend packet now because we recieved ack
             expectedAck++; //update because we expected next sequence number
             machineState = WAIT_SYN;
          }
          break;

        case WAIT_SYN:
          /*use findPacket to try get packets with
           * SYN flags on them*/
          packet = findPacket (recvPackets,-1,SYN);
          if(recievedSYN(packet)){
            printf("recieved SYN\n");
            packet = prepareSYN_ACK (packet.head.seq);
            machineState = SET_CHECKSUM;
            //NOTE TODO set the specific sequence numbers and windowsize to what server wants
          }
          break;
        case EST_CONN:

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

  pthread_join(writeToServer, NULL);
  pthread_join(readFromServer, NULL);

}
