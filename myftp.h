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
int changeDirectory(char path[]);
void deleteFile(char direct[]);
void makeDirectory();
void printWorkingDirectory();
void getFile();
void putFile();

#endif
