#ifndef CLIENT_STUFF
#define CLIENT_STUFF

#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <signal.h>
#include <netdb.h>
#include <unordered_map>
#include <pthread.h>

#define BUFFSIZE 100
//socket file descriptor
//needs to be global so signal handler can access it
inline int sockfd, terminate_sock;
inline unsigned int cid;
inline pthread_t tid;

inline bool done_sending = true;

//string to int mappings
inline std::unordered_map<std::string, int> code = {
	{"quit", 1}, {"get", 2}, {"put", 3}, {"delete", 4}, {"ls", 5}, 
	{"pwd", 6}, {"cd", 7}, {"mkdir", 8}, {"terminate", 9} };

//function prototypes/signatures
void errexit(const std::string message);
void handleGetCommand(int sockfd, const std::string &input);
void handlePutCommand(const int &sockfd, const std::string &input);
void *handleGetBackground(void* arg);
void *handlePutBackground(void* arg);

//signal handler
void handler(int num);

#endif