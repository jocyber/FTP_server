#ifndef CLIENT_HANDLE
#define CLIENT_HANDLE

#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <stdlib.h>
#include "myftp.h"

inline unsigned int commandID = 0;
inline pthread_mutex_t commandID_lock;
inline pthread_mutex_t hashTableLock;
inline std::unordered_map<unsigned int, bool> globalTable;

void* handle_client(void* port);
void* handle_termination(void* port);

//exit the program on critical error
void exitFailure(const std::string str);

#endif