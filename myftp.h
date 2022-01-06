#ifndef FTP
#define FTP

#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#define BUFFSIZE 512

void listDirectories(char message[]);
void getFile();
void putFile();

#endif
