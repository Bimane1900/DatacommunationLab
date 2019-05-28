/*
 * Simon Roysson and Mareks Ozols
 * 
 * header.h - contains all definitions for flags
 * ports, sizes and states. Some definitions can
 * be change to alter behavior of program.
 * (TIMEOUT, FAIL_THRESHOLD, ServWinSize).
 * Also contains needed function and struct
 * declaration for the program to work*/

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
#define BUFFSIZE 10
#define INVALID_SEQ -1

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
#define WAIT_PKT 16


//definitions used in program
//can be changed to get different results
#define ClientWinSize 8
#define ServWinSize 4
#define TIMEOUT 5
#define FAIL_THRESHOLD 30

struct header{
  int flags;
  int id;
  int seq;
  int windowsize;
  int crc;
  int length2;
  int timestamp;
};

typedef struct rtp_struct{
  struct header head;
  char *data;
}rtp;

void initSocketAddress(struct sockaddr_in *name, char *hostName, unsigned short int port);
void writeMessage(int fileDescriptor, rtp packet, struct sockaddr_in serverName);
int readMessageFrom(int fileDescriptor, rtp* packet, struct sockaddr_in6* address);
void* readInput (void* input);
void* readServerMessage (void* fileDescriptor);
int makeSocket(unsigned short int port);
char* serialize_UDP( rtp udp);
rtp deserialize_UDP(char* buffer);
int Checksum (rtp packet);
rtp prepareSYNpkt(int winSize, int seq);
rtp prepareSYN_ACK (int seq);
rtp prepareACK(int seq);
rtp prepareNACK(int expectedPktSeq);
rtp preparePKT(char* msgToSend, int sendingSeq);
rtp prepareFIN(int sendingSeq);
rtp prepareFIN_ACK(int sendingSeq);
int recievedSYN_ACK(rtp packet);
int recievedSYN(rtp packet);
int receivedPKT(rtp packet);
int recievedFIN(rtp packet);
int recievedFIN_ACK(rtp packet);
int expectedPKT(rtp packet, int expectedPkt);
int expectedACK(rtp packet, int expectedAck);
int acceptablePKT(rtp packet, int acceptList[(ServWinSize/2)-1]);
int acceptableACK(rtp packet, int acceptList[(ServWinSize/2)-1]);
int validNACK(rtp packet, int acceptList[(ServWinSize/2)-1], int expected);
void initBuffer(rtp buff[BUFFSIZE]);
int push(rtp buff[BUFFSIZE], rtp packet);
rtp findPacket(rtp buff[BUFFSIZE], int seq, int flags);
rtp findNewPKT(rtp buff[BUFFSIZE]);
void pop(rtp buff[BUFFSIZE], int seq);
void printBuff(rtp buff[BUFFSIZE]);
void updateSendSeq(int* sendSeq,int winsize, int startSeq);
void updateAcceptablePKTs(int acceptablePkts[], int size, int slideWin, int startSeq);
void slideWindow(int *expectedPKT, int acceptList[],int slideWin, int startSeq);
void resendTimeouts(rtp buff[BUFFSIZE],int sock, struct sockaddr_in serverName);
int windowIsFull(int sendSeq, int expected, int windowsize);

#endif
