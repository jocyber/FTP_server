#include "client_handlers.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <exception>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/file.h>

//read the current directory
void listDirectories(const int &client_sock) {
  char path[] = "./";
	char space[] = "\n";
  char message[BUFFSIZE];
  memset(message, '\0', BUFFSIZE);

	DIR *direct = opendir(path);
	struct dirent *dirP;	

	while((dirP = readdir(direct)) != NULL) {
		if(strcmp(dirP->d_name, ".") == 0 || strcmp(dirP->d_name, "..") == 0)
			continue;

		strcat(message, dirP->d_name);
		strcat(message, space);
	}

 if(send(client_sock, message, sizeof(message), 0) == -1) 
   throw "Can't send ls to client";
   
}

//send file to the client
void getFile(const std::string &file, const int &client_sock, unsigned int cid) {
	int fd;
	// check if file exists on the server
	if((fd = open(file.c_str(), O_RDONLY)) == -1) {
		char errorMsg[] = "File does not exist.\n";

		if(send(client_sock, errorMsg, sizeof(errorMsg), 0) == -1)
			throw "Failed to send error msg to client.\n";
	
		return;
	}

	char successMsg[] = "file exists";
	if(send(client_sock, successMsg, sizeof(successMsg), 0) == -1)
			throw "Failed to send error msg to client.\n";

	struct stat sb;
	fstat(fd, &sb);
	// send file size
	int fileSize = sb.st_size;
	if(send(client_sock, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to send the file size.\n";

	int bytesSent = 0;
	char buffer[BUFFSIZE];
  // create a shared lock on file
  flock(fd, LOCK_SH);
	while(bytesSent < fileSize) {
		int bytesRead = read(fd, buffer, BUFFSIZE);

		if(send(client_sock, buffer, bytesRead, 0) == -1)
			throw "Failed to send 'ls' message to client.\n";

		if(globalTable[cid]) {
			break;
		}
		bytesSent += bytesRead;
    usleep(10);
	}

  // remove shared lock
  flock(fd, LOCK_UN);
	if(close(fd) == -1)
		throw "Failed to close the file.\n";
}

void putFile(const std::string &filename, const int &client_sock, unsigned int cid, char buffer[], bool &commandAlreadyRecieved) {
  // create file
	int fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0666);
	char output[BUFFSIZE];

	if(fd == -1) 
		throw "Failed to open file.\n";

	// get file Size from server
	int fileSize;
	if(recv(client_sock, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to receive data from the client.\n";
	
	// keep reciving data until there's none left
	int bytesLeft = fileSize;
	int bytesRecieved;
  // put an exclusive lock on the file
  int lock;
  bool waitedInLock = false;
  while((lock = flock(fd, LOCK_EX | LOCK_NB)) == -1) {
    waitedInLock = true;
  }
  if(waitedInLock) {
    // delete file and reopen
    if(close(fd) == -1)
		  throw "Failed to close the file.\n";
    remove(filename.c_str());
    
    fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0666);
  }
  std::cout << "Lock " << pthread_self() << ": " << lock << "\n";
	while(bytesLeft > 0) {
		memset(output, 0, BUFFSIZE);

		if((bytesRecieved = recv(client_sock, output, BUFFSIZE, 0)) == -1)
			throw "Failed to receive data from the client.\n";
		if(bytesRecieved > 0) {
			if(write(fd, output, bytesRecieved) != bytesRecieved)
				throw "Write error to file.\n";
		}
   
	  // if terminated, stop reading
		if(globalTable[cid]) {
			// remove file
      std::cout << "Exiting: " << output << "\n";
      strcpy(buffer, output);
      std::cout << "Copied: " << buffer << "\n";
      commandAlreadyRecieved = true;
			remove(filename.c_str());
			break;
		}

		bytesLeft -= bytesRecieved;
	}
  
  // remove file lock
  flock(fd, LOCK_UN);
  std::cout << "Lock " << pthread_self() << " released\n";
  
	if(close(fd) == -1)
		throw "Failed to close the file.\n";
}
