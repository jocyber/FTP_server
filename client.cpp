#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <fcntl.h>
#include <fstream>
#include <sys/stat.h>
#include <sys/sendfile.h>

#define BUFFSIZE 512
using uint = unsigned int;

void errexit(const std::string message);
void handleGetFile(int sockfd, char output[], std::string inputSubstring);
void handlePutFile(int sockfd, char output[], std::string inputSubstring);

int main(int argc, char **argv)
{
	if(argc != 3)
		errexit("Format: ./client {server_domain_name} {port_number}\n");

	//command-line argument information
	//char *name = argv[1]; domain hasn't been implemented yet
	uint port_num = atoi(argv[2]);

	int sockfd;
	std::string input;
	std::string inputSubstring;

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

		// convert input into a stringstream to read word by word
		std::stringstream ss;
		ss << input;
		ss >> inputSubstring;
		

		// check if file already exists on local pc, prevent server from sending file over
		if(inputSubstring.compare("get") == 0) {
			ss >> inputSubstring;
			inputSubstring = "copied_" + inputSubstring;  // used for testing on own computer to be able to duplicate file with another name, since always working in same directory
			
			std::ifstream file;
			file.open(inputSubstring);
			if(file.is_open()) {
				std::cout << "File already exists.\n";
				continue;
			}
			file.close();
		}
		// check to make sure file exists on computer
		if(inputSubstring.compare("put") == 0) {
			ss >> inputSubstring;
			
			std::ifstream file;
			file.open(inputSubstring);
			if(!file.is_open()) {
				std::cout << "File does not exist.\n";
				continue;
			}
			file.close();
		}

		//guard against buffer overflow
		if(input.length() > BUFFSIZE)
			std::cerr << "Buffer overflow.\n";
		else {
			// send command to server
			send(sockfd, input.c_str(), BUFFSIZE, 0);

			// handle getting file
			if(input.substr(0,3) == "get") {
				handleGetFile( sockfd, output, inputSubstring);
				ss.clear();
				continue;
			}

			// handle puting file
			if(input.substr(0,3) == "put") {
				handlePutFile( sockfd, output, inputSubstring);
				ss.clear();
				continue;
			}
		
			if(recv(sockfd, output, BUFFSIZE, 0) == -1)
				errexit("Failed to receive data from the server.");

			if(strcmp(output, "quit") == 0)
				break;

			std::cout << output << '\n';
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

void handleGetFile(int sockfd, char output[], std::string inputSubstring) {
	// check to see if file was found on server
	if(recv(sockfd, output, BUFFSIZE, 0) == -1)
		errexit("Failed to receive data from the server.");
				
	if(strcmp(output, "file not found") == 0) {
		std::cout << "File was not found.\n";
		return;
	}

	// get permission of file from server
	struct stat fileStat;
	if(recv(sockfd, &fileStat, sizeof(fileStat), 0) == -1)
		errexit("Failed to receive data from the server.");

	// create file
	int file_fd = open(inputSubstring.c_str(), O_CREAT | O_APPEND | O_WRONLY, 0666 );
	if( file_fd < 0) {
		std::cout << "Error opening file.\n";
		return;
	}

	// copy to file
	int bitsLeft = fileStat.st_size;
	int bitsRecieved;
	while(bitsLeft > 0) {
		memset(output, 0, BUFFSIZE);
		if((bitsRecieved = recv(sockfd, output, BUFFSIZE, 0)) == -1)
			errexit("Failed to receive data from the server.");
		if(write(file_fd, output, sizeof(char) * bitsRecieved) != bitsRecieved) {
			std::cout << "Write error to file.";
			return;
		}
		bitsLeft -= bitsRecieved;
		output[0] = '\0';
	}

	close(file_fd);
}

void handlePutFile(int sockfd, char output[], std::string inputSubstring) {
	// check to see if file was not already created on the server
	if(recv(sockfd, output, BUFFSIZE, 0) == -1)
		errexit("Failed to receive data from the server.");
				
	if(strcmp(output, "file exists") == 0) {
		std::cout << "File already exists on server.\n";
		return;
	}

	// open file
	int file_fd = open(inputSubstring.c_str(), O_RDONLY );
	if(file_fd < 0) {
		std::cout << "Error opening file.\n";
		return;
	}

	// send file, and permissions
	struct stat fileStat;
	fstat(file_fd, &fileStat);
	send(sockfd, &fileStat, sizeof(fileStat), 0);
	sendfile(sockfd, file_fd, 0, fileStat.st_size);
}