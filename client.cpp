#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <netinet/tcp.h>
#include <sys/sendfile.h>
#include <signal.h>
#include <netdb.h>
#include <unordered_map>

#define BUFFSIZE 1000
//socket file descriptor
//needs to be global so signal handler can access it
int sockfd;

//string to int mappings
std::unordered_map<std::string, int> code = {
	{"quit", 1}, {"get", 2}, {"put", 3}, {"delete", 4}, {"ls", 5}, 
	{"pwd", 6}, {"cd", 7}, {"mkdir", 8} };

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

	int option;
	//define signal handler for the kill signal(Cntl-C)
	signal(SIGINT, handler);//handler is a function pointer

	while(true) {
		try {
			input.clear();//O(1)
			memset(output, '\0', BUFFSIZE);//clear array (fastest standard way)
			std::cout << "myftp> ";

			std::getline(std::cin, input);

			//guard against buffer overflow
			if(input.length() > BUFFSIZE)
				throw "Buffer overflow error.";
			else {
				//get the command type
				std::string req = "";

				for(unsigned int i = 0; input[i] != ' ' && i < input.length(); ++i)
					req += input[i];

				if(code.find(req) == code.end())
					option = -1;
				else
					option = code[req];

				//switch on result
				switch(option) {
					case 2: {//get
						//check for files existence
						std::string file = input.substr(4, input.length());

						// check if the file already exists in current directory
						FILE* fp;
						fp = fopen(file.c_str(), "r");

						if(fp) {
							fclose(fp);
							throw "File already exists in current directory, won't overwrite.\n";
						}

						if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
							throw "Failed to send the file for 'get' to the server.";

						// check if file exists on server
						char fileMessage[100];
						if(recv(sockfd, fileMessage, sizeof(fileMessage), 0) == -1)
							throw "Failed to receive data from the server.";

						if(strcmp(fileMessage, "file exists") != 0)
							throw "File does not exist on the server.";

						handleGetCommand(sockfd, file);
						break;
					}
					case 3: {//put
						std::string fileName = input.substr(4, input.length());

						// check to make sure file exists on the computer					
						FILE* fp;
						fp = fopen(fileName.c_str(), "r");
						if(fp == NULL)
							throw "File does not exist in current directory.\n";

						fclose(fp);
						//send the file name
						if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
							throw "Failed to send the file name to the server.";

						//send size and contents
						handlePutCommand(sockfd, input.substr(4, input.length()));
						break;
					}
					case 1: {//quit
						char quit_mess[] = "quit";
						if(send(sockfd, quit_mess, sizeof(quit_mess), 0) == -1)
							throw "Failed to send the input to the server.";

						if(close(sockfd) == -1)
							errexit("Failed to close the socket.");

						return EXIT_SUCCESS;
					}

					case 5: { // ls
						int recv_size;
						
						if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
							throw "Failed to send the input to the server.";

						while(true) {
							memset(output, '\0', BUFFSIZE);

							if((recv_size = recv(sockfd, output, BUFFSIZE, 0)) > 1)
								std::cout << output;
							else
								break;
						}

						std::cout << '\n';
						break;
					}	

					default:
						if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
							throw "Failed to send the input to the server.";

						if(recv(sockfd, output, BUFFSIZE, 0) == -1)
							throw "Failed to receive data from the server.";

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

	// get fileSize from server
	int fileSize;
	if(recv(sockfd, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to receive data from the server.";
	
	//keep reciving data until there's none left
	int bytesReceived = 0, bytesLeft = fileSize;
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
	// check to make sure file dosen't exist on server
	char fileMessage[100];
	if(recv(sockfd,	fileMessage, sizeof(fileMessage), 0) == -1)
		throw "Failed to receive data from the server.";

	if(strcmp(fileMessage, "file does not exist") != 0)
		throw "File already exists on the server.";

	int fd;
	if((fd = open(file.c_str(), O_RDONLY)) == -1)
			throw "Failed to open file.";

	struct stat sb;
	fstat(fd, &sb);

	//send file size to server
	int fileSize = sb.st_size;
	if(send(sockfd, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to send file's metadata.";

	if(sendfile(sockfd, fd, 0, sb.st_size) == -1)
		throw "Failed to send file's contents.";

	if(close(fd) == -1)
		throw "Failed to close the file.";
}

