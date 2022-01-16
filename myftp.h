#ifndef FTP
#define FTP

#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string>
#include <fcntl.h>
#include <sys/sendfile.h>
#include <sys/socket.h>

#define BUFFSIZE 512

void listDirectories(char message[]);
int getFile(char message[], std::string filename, int client_sock, char buffer[]);
int putFile(char message[], std::string filename, int client_sock, char buffer[]);

#endif
