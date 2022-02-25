#include "client_handlers.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <exception>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>

//read the current directory
void listDirectories(const int &client_sock) {
	FILE *fd = popen("ls", "r");

	if(fd == NULL)
		throw "Failed to execute 'ls' on the command-line.";

	char buffer[BUFFSIZE];
	memset(buffer, '\0', BUFFSIZE);

	while(fgets(buffer, BUFFSIZE, fd) != NULL) {
		if(send(client_sock, buffer, strlen(buffer), 0) == -1)
			throw "Failed to send 'ls' message to client.\n";
	}

	fclose(fd);

	// char temp[] = "";
	// if(send(client_sock, temp, sizeof(temp), 0) == -1)
	// 	throw "Failed to send 'ls' message to client.\n";
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

	//place a lock on the open file.
	if(flock(fd, LOCK_EX) == -1) {//lock_ex is an exclusive lock
		close(fd);
		throw "Failed to place a lock on the file upon 'get' command.";
	}
		
	char successMsg[] = "file exists";
	if(send(client_sock, successMsg, sizeof(successMsg), 0) == -1)
			throw "Failed to send error msg to client.\n";

	//int bytesSent = 0;
	char buffer[BUFFSIZE];
	memset(buffer, '\0', BUFFSIZE);

	ssize_t recv_size;		

	while((recv_size = read(fd, buffer, BUFFSIZE)) > 0) {
		if(globalTable[cid]) {
			memset(buffer, '\0', BUFFSIZE);
			break;
		}

		if(send(client_sock, buffer, recv_size, 0) != recv_size)
			throw "Error in receiving data from the socket.";

		//sleep(1);
	}

	//unlock the file
	if(flock(fd, LOCK_UN) == -1) {
		close(fd);
		throw "Failed to unlock the file upon 'get' command.";
	}

	if(close(fd) == -1)
		throw "Failed to close the file.\n";
}

//code for the put command
void putFile(const std::string &filename, int client_sock, unsigned int cid) {
	int fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0666);
	if(fd == -1) 
		throw "Failed to open file.\n";

	//place lock on the file
	if(flock(fd, LOCK_EX) == -1) {
		close(fd);
		throw "Failed to place a lock on the file upon 'put' command.";
	}
	
	//keep receiving data until there's none left
	bool remove_file = false;
	char output[BUFFSIZE];
	memset(output, '\0', BUFFSIZE);

	//recieve data from the socket
	ssize_t numRead;
	while((numRead = read(client_sock, output, BUFFSIZE)) > 0) {
		if(write(fd, output, numRead) != numRead)
			throw "Failed to write data to the file.";

		if(globalTable[cid]) {
			remove_file = true;
			break;
		}

		if(numRead < BUFFSIZE)
			break;

		//sleep(1);
	}

	/********* clean up *********/
	//unlock the file with flock
	if(flock(fd, LOCK_UN) == -1) {
		close(fd);
		throw "Failed to unlock the file upon 'put' command.";
	}

	if(close(fd) == -1)
		throw "Failed to close the file.\n";

	if(remove_file) {
		remove(filename.c_str());

		//clear out socket
		char tempBuff[BUFFSIZE];
		memset(tempBuff, '\0', BUFFSIZE);
		//non-blocking recieve to clear out remaining data in socket

		fcntl(client_sock, F_SETFL, O_NONBLOCK); // set socket to non blocking
		for(int i = 0; i < 5; i++) {
			//sleep(1);
			recv(client_sock, tempBuff, BUFFSIZE, 0); 
		}
	
		// set socket back to blocking
		int oldfl = fcntl(client_sock, F_GETFL);
		fcntl(client_sock, F_SETFL, oldfl & ~O_NONBLOCK); // unset
	}
}
