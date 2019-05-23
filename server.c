
#include "header.h"

int main(int argc, char *argv[]) {
  int sock;
  int sendSock;
  int clientSocket;
  int i;
  struct sockaddr_in clientName;
  socklen_t size;
  char* ipAddr;


  /* Create a socket and set it up to listen */
  sock = makeSocket(PORT);

  /*Hardcoded client address*/
  initSocketAddress (&clientName,"127.0.0.1",PORT+1);

  /*Listen for messages and reply*/
  while(1){
    if(readMessageFrom(sock))
      writeMessage(sock,"I hear you...",clientName);
  }

}
