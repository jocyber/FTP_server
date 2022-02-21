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
	while(fgets(buffer, BUFFSIZE, fd) != NULL) {
		if(send(client_sock, buffer, BUFFSIZE, 0) == -1)
			throw "Failed to send 'ls' message to client.\n";

		memset(buffer, '\0', sizeof(buffer));
	}

	fclose(fd);

	char temp[] = "";
	if(send(client_sock, temp, sizeof(temp), 0) == -1)
		throw "Failed to send 'ls' message to client.\n";
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
	if(flock(fd, LOCK_SH) == -1) {//lock_sh is a shared lock for read-only files
		close(fd);
		throw "Failed to place a lock on the file upon 'get' command.";
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
	
	//take in data from the socket
	while(bytesSent < fileSize) {
		int bytesRead = read(fd, buffer, BUFFSIZE);

		if(send(client_sock, buffer, bytesRead, 0) == -1)
			throw "Failed to send 'ls' message to client.\n";

		if(globalTable[cid])
			break;
		
		bytesSent += bytesRead;
		//sleep(1);//temporary for testing purposes
	}

	//unlock the file
	if(flock(fd, LOCK_UN) == -1) {
		close(fd);
		throw "Failed to unlock the file upon 'get' command.";
	}

	if(close(fd) == -1)
		throw "Failed to close the file.\n";
}

void putFile(const std::string &filename, const int &client_sock, unsigned int cid) {
	//check if file already exists
	FILE *fp;

	if((fp = fopen(filename.c_str(), "r")) != NULL) {
		char existsMsg[] = "File already exists on server.\n";
		fclose(fp);

		if(send(client_sock, existsMsg, sizeof(existsMsg), 0) == -1)
			throw "Failed to send error message to client.\n";

		return;
	}
	else {
		char fileSuccess[] = "file does not exist";
		if(send(client_sock, fileSuccess, sizeof(fileSuccess), 0) == -1)
			throw "Failed to send success message to client.\n";
	}

	int fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0666);
	char output[BUFFSIZE];

	if(fd == -1) 
		throw "Failed to open file.\n";

	//place lock on the file
	if(flock(fd, LOCK_EX) == -1) {
		close(fd);
		throw "Failed to place a lock on the file upon 'put' command.";
	}

	// get file Size from server
	int fileSize;
	if(recv(client_sock, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to receive data from the client.\n";
	
	//keep reciving data until there's none left
	int bytesLeft = fileSize;
	int bytesReceived;

	while(bytesLeft > 0) {
		memset(output, 0, BUFFSIZE);

		if((bytesReceived = recv(client_sock, output, BUFFSIZE, 0)) == -1)
			throw "Failed to receive data from the client.\n";
		if(bytesReceived > 0) {
			if(write(fd, output, bytesReceived) != bytesReceived)
				throw "Write error to file.\n";
		}
		
		// if terminated, stop reading
		if(globalTable[cid]) {
			remove(const_cast<char*>(filename.c_str()));
			break;
		}

		bytesLeft -= bytesReceived;
	}

	//unlock the file with flock
	if(flock(fd, LOCK_UN) == -1) {
		close(fd);
		throw "Failed to unlock the file upon 'put' command.";
	}

	if(close(fd) == -1)
		throw "Failed to close the file.\n";
}
