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

void listDirectories(char message[]);
ssize_t getFile(const std::string &file, const int &client_sock);
void putFile();

#endif
