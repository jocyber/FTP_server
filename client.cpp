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

#define BUFFSIZE 512
using uint = unsigned int;

void errexit(const std::string message);
void handleGetCommand(const int &sockfd, const std::string &file, char buffer[], int &fd);

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
			send(sockfd, input.c_str(), BUFFSIZE, 0);
			//if input is 'get': while recv != -1, output to file
			if(input.substr(0,3).compare("get") == 0) {
				//check for files existence
				std::string file = input.substr(4, input.length());
				int fd = open(file.c_str(), O_WRONLY | O_CREAT, 0666);

				if(fd == -1)
					std::cerr << "Failed to open file or already exists.\n";
				else
					handleGetCommand(sockfd, input, output, fd);

				if(close(fd) == -1)
					std::cerr << "Failed to close the file.\n";
			}
			else {
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

void handleGetCommand(const int &sockfd, const std::string &file, char output[], int &fd) {
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
}
