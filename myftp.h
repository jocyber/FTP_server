#ifndef FTP
#define FTP

#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>

#define BUFFSIZE 512

//reads the current directory
void listDirectories(char message[]);
//pulls the file from the server to the client
ssize_t getFile(const std::string &file, const int &client_sock);
//pulls the file from the client to the server 
void putFile();

#endif
