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
#include <netdb.h>

#define BUFFSIZE 512
using uint = unsigned int;

//socket file descriptor
int sockfd;
//needs to be global so signal handler can access it

//function prototypes
void errexit(const std::string message);
void handleGetCommand(const int &sockfd, const std::string &file);
void handlePutCommand(const int &sockfd, const std::string &file);

//signal handler
void handler(int num) {
	char signal_message[] = "quit_signal";
	send(sockfd, signal_message, sizeof(signal_message), 0);

	printf("\n");
	exit(1);
}

int main(int argc, char **argv) {
	if(argc != 3)
		errexit("Format: ./client {server_domain_name} {port_number}\n");

	//command-line argument information
	uint port_num = atoi(argv[2]);
	char *domain = argv[1];

	// get ip-address of the domain
	struct hostent *hostInfo;
	struct in_addr **addrList;

	if((hostInfo = gethostbyname(domain)) == NULL)
		errexit("Domain name not found.");

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		errexit("Failed to create a socket.");

	struct sockaddr_in addr;
	memset((struct sockaddr*) &addr, 0, sizeof(addr));
	addrList = (struct in_addr **)hostInfo->h_addr_list;

	char domainIP[100];
	strcpy(domainIP, inet_ntoa(*addrList[0]));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_num);//network byte ordering of port number
	addr.sin_addr.s_addr = inet_addr(domainIP);

	char output[BUFFSIZE] = "";
	std::string input;

	if((connect(sockfd, (struct sockaddr*) &addr, sizeof(addr))) == -1)
		errexit("Could not connect to the socket.");

	//define signal handler for the kill signal(Cntl-C)
	signal(SIGINT, handler);//handler is a function pointer

	while(true) {
		try {
			input.clear();//O(1)
			memset(output, '\0', BUFFSIZE);
			std::cout << "myftp> ";

			std::getline(std::cin, input);

			//guard against buffer overflow
			if(input.length() > BUFFSIZE)
				throw "Buffer overflow error.";
			else {
				if(input.substr(0,3).compare("get") == 0) {
					//check for files existence
					std::string file = input.substr(4, input.length());

					if(access(file.c_str(), F_OK) == 0)
						throw "File already exists in the current directory.";
					else {
						if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
							throw "Failed to send the file for 'get' to the server.";

						// check if file exists on server
						char fileMessage[100];
						if(recv(sockfd, fileMessage, sizeof(fileMessage), 0) == -1)
							throw "Failed to receive data from the server.";

						if(strcmp(fileMessage, "file exists") != 0)
							throw "File does not exist.";

						handleGetCommand(sockfd, file);
						std::cout << '\n';
					}
				}
				else if(input.substr(0,3).compare("put") == 0) {
					// check to make sure file exists on the computer
					std::string fileName = input.substr(4, input.length());
					
					if(access(fileName.c_str(), F_OK) == -1) {
						std::string error = "File {" + fileName + "} does not exist.\n";
						throw error.c_str();
					}

					//send the file name
					if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
						throw "Failed to send the file name to the server.";

					//send size and contents
					handlePutCommand(sockfd, input.substr(4, input.length()));
				}
				else {
					if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
						throw "Failed to send the input to the server.";

					if(recv(sockfd, output, BUFFSIZE, 0) == -1)
						throw "Failed to receive data from the server.";

					if(strcmp(output, "quit") == 0)
						break;

					std::cout << output << '\n';
				}
			}
		}
		catch(const char* message) {
			std::cerr << message << '\n';
		}
		catch(...) {
			std::cerr << "Unknown error occured.\n";
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
void handleGetCommand(const int &sockfd, const std::string &file) {
	// create file on local computer
	int fd = open(file.c_str(), O_WRONLY | O_CREAT, 0666);
	if(fd == -1)
		throw "Failed to open file.";

	// get permission of file from server
	struct stat fileStat;
	if(recv(sockfd, &fileStat, sizeof(fileStat), 0) == -1)
		throw "Failed to receive data from the server.";
	
	//keep reciving data until there's none left
	int bytesReceived = 0, bytesLeft = fileStat.st_size;
	char output[BUFFSIZE];

	while(bytesLeft > 0) {
		memset(output, '\0', BUFFSIZE);

		if((bytesReceived = recv(sockfd, output, BUFFSIZE, 0)) == -1)
			 throw "Failed to receive data from the server.";
		if(write(fd, output, bytesReceived) != bytesReceived)
			throw "Write error to file.";

		bytesLeft -= bytesReceived;
	}

	if(close(fd) == -1)
		throw "Failed to close the file.";
}

//upload file to server
void handlePutCommand(const int &sockfd, const std::string &file) {
	// check to make sure file existed on the server
	char fileMessage[100];
	if(recv(sockfd,	fileMessage, sizeof(fileMessage), 0) == -1)
		throw "Failed to receive data from the server.";

	if(strcmp(fileMessage, "file does not exist") != 0)
		throw "File already exists on the server";

	int fd;
	if((fd = open(file.c_str(), O_RDONLY)) == -1)
			throw "Failed to open file.";

	struct stat sb;
	fstat(fd, &sb);

	//send file size first
	if(send(sockfd, &sb, sizeof(sb), 0) == -1)
		throw "Failed to send file's metadata.";

	if(sendfile(sockfd, fd, 0, sb.st_size) == -1)
		throw "Failed to send file's contents.";
}

