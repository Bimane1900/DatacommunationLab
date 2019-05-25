#ifndef HEADER_FILE
#define HEADER_FILE
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <time.h>

#define PORT 5555
#define hostNameLength 50
#define messageLength  256
#define MAXMSG 512
#define BUFFSIZE 5

#define SYN 1
#define SYN_ACK 2
#define ACK 4
#define FIN 8
#define FIN_ACK 16
#define NACK 32

#define CONN_SETUP 0
#define SLIDING_WINDOW 1
#define CONN_TEARDOWN 2

#define INIT 0
#define SET_CHECKSUM 1
#define WAIT_SYN_ACK 2
#define READ_CHECKSUM 3
#define EST_CONN 4
#define WAIT_FIN_ACK 5
#define WAIT_FIN 6
#define WAIT 7
#define CLOSED 8
#define WINDOW_NOT_FULL 9
#define CHECK_WINDOW_SIZE 10
#define WINDOW_FULL 11
#define READ_SEQ_NUMBER 12
#define READ_BUFFER 13
#define WAIT_SYN 14
#define WAIT_ACK 15

#define ClientWinSize 8
#define ServWinSize 6

typedef struct threadArgument{
  int fileDescriptor;
  struct sockaddr_in server;
}argument;

struct header{
  int flags;
  int id;
  int seq;
  int windowsize;
  int crc;
  int length2;
};

typedef struct rtp_struct{
  struct header head;
  char *data;
}rtp;

void initSocketAddress(struct sockaddr_in *name, char *hostName, unsigned short int port);
void writeMessage(int fileDescriptor, rtp packet, struct sockaddr_in serverName);
int readMessageFrom(int fileDescriptor, rtp* packet);
void* readInput (void* input);
void* readServerMessage (void* fileDescriptor);
int makeSocket(unsigned short int port);
char* serialize_UDP( rtp udp);
rtp deserialize_UDP(char* buffer);
int Checksum (rtp packet);
rtp prepareSYNpkt(int winSize);
rtp prepareSYN_ACK (int seq);
int recievedSYN_ACK(rtp packet);
int recievedSYN(rtp packet);
void initBuffer(rtp buff[BUFFSIZE]);
int push(rtp buff[BUFFSIZE], rtp packet);
rtp findPacket(rtp buff[BUFFSIZE], int seq, int flags);
void pop(rtp buff[BUFFSIZE], int seq);



#endif
