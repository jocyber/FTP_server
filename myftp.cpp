#include "myftp.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <exception>

//read the current directory
void listDirectories(char message[]) {
	char path[] = "./";
	char space[] = " ";

	DIR *direct = opendir(path);
	struct dirent *dirP;	

	while((dirP = readdir(direct)) != NULL) {
		if(strcmp(dirP->d_name, ".") == 0 || strcmp(dirP->d_name, "..") == 0)
			continue;

		strcat(message, dirP->d_name);
		strcat(message, space);
	}
}

//send file to the client
void getFile(const std::string &file, const int &client_sock) {
	int fd;
	if((fd = open(file.c_str(), O_RDONLY)) == -1)
		throw "Failed to open file.\n";

	struct stat sb;
	stat(file.c_str(), &sb);
	if(send(client_sock, &sb, sizeof(sb), 0) == -1)
		throw "Failed to send the file size.\n";

	int optval = 1;
	//enable tcp_cork (only send a packet when it's full, meaning network congestion is reduced)
	if(setsockopt(client_sock, IPPROTO_TCP, TCP_CORK, &optval, sizeof(optval)) == -1)
		throw "Failed to enable TCP_CORK.\n";

	if(sendfile(client_sock, fd, 0, sb.st_size) == -1)
		throw "Failed to send file contents.\n";
	//disable tcp_cork
	optval = 0;
	if(setsockopt(client_sock, IPPROTO_TCP, TCP_CORK, &optval, sizeof(optval)) == -1)
		throw "Failed to disable TCP_CORK.\n";
	
	if(close(fd) == -1)
		throw "Failed to close the file.\n";
}

void putFile(const std::string &filename, const int &client_sock) {
	int fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0666);
	char output[BUFFSIZE];

	if(fd == -1) 
		std::cout << "Failed to open file.\n";

	// get permission of file from server
	struct stat fileStat;
	if(recv(client_sock, &fileStat, sizeof(fileStat), 0) == -1)
		throw "Failed to receive data from the client.\n";
	
	//keep reciving data until there's none left
	int bytesLeft = fileStat.st_size;
	int bytesRecieved;

	while(bytesLeft > 0) {
		memset(output, 0, BUFFSIZE);

		if((bytesRecieved = recv(client_sock, output, BUFFSIZE, 0)) == -1)
			throw "Failed to receive data from the client.\n";
		if(write(fd, output, bytesRecieved) != bytesRecieved)
			throw "Write error to file.\n";

		bytesLeft -= bytesRecieved;
	}

	if(close(fd) == -1)
		throw "Failed to close the file.\n";
}
