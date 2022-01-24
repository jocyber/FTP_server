#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>

#define BUFFSIZE 512
using uint = unsigned int;

void errexit(const std::string message);
void handleGetCommand(const int &sockfd, const std::string &file, char buffer[]);
void handlePutCommand(const int &sockfd, const std::string &file);

int main(int argc, char **argv)
{
	if(argc != 3)
		errexit("Format: ./client {server_domain_name} {port_number}\n");

	//command-line argument information
	//char *name = argv[1]; domain hasn't been implemented yet
	uint port_num = atoi(argv[2]);

	int sockfd;
	std::string input;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		errexit("Failed to create a socket.");

	struct sockaddr_in addr;
	memset((struct sockaddr*) &addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_num);//network byte ordering of port number
	addr.sin_addr.s_addr = INADDR_ANY;//IPv4 'wildcard'

	char output[BUFFSIZE] = "";

	if((connect(sockfd, (struct sockaddr*) &addr, sizeof(addr))) == -1)
		errexit("Could not connect to the socket.");

	while(true) {
		//empty the string
		input.clear();
		output[0] = '\0';
		std::cout << "myftp> ";

		std::getline(std::cin, input);

		//guard against buffer overflow
		if(input.length() > BUFFSIZE)
			std::cerr << "Buffer overflow.\n";
		else {
			if(input.substr(0,3).compare("get") == 0) {
				//check for files existence
				std::string file = input.substr(4, input.length());

				if(access(file.c_str(), F_OK) == 0)
					std::cerr << "File already exists in the current directory.\n";
				else {
					send(sockfd, input.c_str(), BUFFSIZE, 0);
					handleGetCommand(sockfd, file, output);
				}
			}
			else if(input.substr(0,3).compare("put") == 0) {
				//send the file name
				send(sockfd, input.c_str(), BUFFSIZE, 0);
				//send size and contents
				handlePutCommand(sockfd, input.substr(4, input.length()));
			}
			else {
				send(sockfd, input.c_str(), BUFFSIZE, 0);

				if(recv(sockfd, output, BUFFSIZE, 0) == -1)
					errexit("Failed to receive data from the server.");

				if(strcmp(output, "quit") == 0)
					break;

				std::cout << output << '\n';
			}
		}
	}

	if(close(sockfd) == -1)
		errexit("Failed to close the socket.");

	return EXIT_SUCCESS;
}

void errexit(const std::string message) {
	std::cerr << message << '\n';
	exit(EXIT_FAILURE);
}

//download file from server
void handleGetCommand(const int &sockfd, const std::string &file, char output[]) {
	int fd = open(file.c_str(), O_WRONLY | O_CREAT, 0666);

	if(fd == -1)
		std::cerr << "Failed to open file or already exists.\n";

	// get permission of file from server
	struct stat fileStat;
	if(recv(sockfd, &fileStat, sizeof(fileStat), 0) == -1)
		errexit("Failed to receive data from the server.");
	
	//keep reciving data until there's none left
	int bytesLeft = fileStat.st_size;
	int bytesRecieved;

	while(bytesLeft > 0) {
		memset(output, 0, BUFFSIZE);

		if((bytesRecieved = recv(sockfd, output, BUFFSIZE, 0)) == -1)
			errexit("Failed to receive data from the server.");
		if(write(fd, output, bytesRecieved) != bytesRecieved) {
			std::cerr << "Write error to file.";
			return;
		}

		bytesLeft -= bytesRecieved;
	}

	if(close(fd) == -1)
		std::cerr << "Failed to close the file.\n";
}

//upload file to server
void handlePutCommand(const int &sockfd, const std::string &file) {
	int fd;
	if((fd = open(file.c_str(), O_RDONLY)) == -1)
			errexit("Failed to open file.\n");

	struct stat sb;
	fstat(fd, &sb);

	//send file size first
	if(send(sockfd, &sb, sizeof(sb), 0) == -1)
		errexit("Failed to send file's metadata.");

	int optval = 1;
	//enable tcp_cork (only send a packet when it's full, meaning network congestion is reduced)
	if(setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &optval, sizeof(optval)))
		errexit("Failed to enable TCP_CORK.");

	if(sendfile(sockfd, fd, 0, sb.st_size) == -1)
		errexit("Failed to send file's contents.");

	//disable tcp_cork
	optval = 0;
	if(setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &optval, sizeof(optval)) == -1)
		errexit("Failed to disable TCP_CORK.");
}

