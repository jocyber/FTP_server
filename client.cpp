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
#include <signal.h>
#include<netdb.h>

#define BUFFSIZE 512
using uint = unsigned int;

//socket file descriptor
int sockfd;
//needs to be global so signal handler can access it

//function prototypes
void errexit(const std::string message);
void handleGetCommand(const int &sockfd, const std::string &file, char buffer[]);
void handlePutCommand(const int &sockfd, const std::string &file);

//signal handler
void handler(int num) {
	char signal_message[] = "quit_signal";
	send(sockfd, signal_message, sizeof(signal_message), 0);

	if(close(sockfd) == -1)
		errexit("Failed to close the socket.");

	std::cout << '\n';
	exit(1);
}


int main(int argc, char **argv)
{
	if(argc != 3)
		errexit("Format: ./client {server_domain_name} {port_number}\n");

	//command-line argument information
	uint port_num = atoi(argv[2]);
	char *domain = argv[1];

	// get ipaddress of the domain
	struct hostent *hostInfo;
	struct in_addr **addrList;

	if((hostInfo = gethostbyname(domain)) == NULL) {
		std::cout << "Domain name not found.\n";
		return 0;
	} // if
	addrList = (struct in_addr **)hostInfo->h_addr_list;
	char domainIP[100];
	strcpy(domainIP, inet_ntoa(*addrList[0]));
	std::cout << "IP: " << domainIP << std::endl;

	std::string input;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		errexit("Failed to create a socket.");

	struct sockaddr_in addr;
	memset((struct sockaddr*) &addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_num);//network byte ordering of port number
	addr.sin_addr.s_addr = inet_addr(domainIP);

	char output[BUFFSIZE] = "";

	if((connect(sockfd, (struct sockaddr*) &addr, sizeof(addr))) == -1)
		errexit("Could not connect to the socket.");

	//define signal handler for the kill signal(Cntl-C)
	signal(SIGINT, handler);//handler is a function pointer

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
					// check if file exists on server
					char fileMessage[100];
					if(recv(sockfd, fileMessage, sizeof(fileMessage), 0) == -1)
						errexit("Failed to receive data from the server.");
					if(strcmp(fileMessage, "file exists") != 0) {
						std::cerr << fileMessage;
						continue;
					}
					handleGetCommand(sockfd, file, output);
				}
			}
			else if(input.substr(0,3).compare("put") == 0) {
				// check to make sure file exists on the computer
				std::string fileName = input.substr(4, input.length());
				if(access(fileName.c_str(), F_OK) == -1) {
					std::cout << "File {" + fileName + "} does not exist.\n";
					continue;
				}
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
	// create file on local computer
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
	memset(output, 0, BUFFSIZE);

	if(close(fd) == -1)
		std::cerr << "Failed to close the file.\n";
}

//upload file to server
void handlePutCommand(const int &sockfd, const std::string &file) {
	// check to make sure file existed on the server
	char fileMessage[100];
	if(recv(sockfd,	fileMessage, sizeof(fileMessage), 0) == -1)
		errexit("Failed to receive data from the server.");

	if(strcmp(fileMessage, "file does not exist") != 0) {
		std::cout << fileMessage;
		return;
	}

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

