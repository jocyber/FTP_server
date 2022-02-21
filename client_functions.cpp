#include "client_functions.h"

void errexit(const std::string message) {
	std::cerr << message << '\n';
	exit(EXIT_FAILURE);
}

void *handleGetBackground(void* arg) {
	try {
		char *temp = (char*)arg;
		
		std::string inputText(temp);
		inputText = inputText.substr(0, inputText.length() - 2);
		handleGetCommand(sockfd, inputText);
	} 
	catch(const char* hello) {
		std::cerr << hello << "\n";
	}

	pthread_exit(0);
}

void *handlePutBackground(void* arg) {
	try {
		char *temp = (char*)arg;

		std::string inputText(temp);
		inputText = inputText.substr(0, inputText.length() - 2);
		handlePutCommand(sockfd, inputText);
	} 
	catch(const char* hello) {
		std::cerr << hello << "\n";
	}

	pthread_exit(0);
}

//download file from server
void handleGetCommand(const int &sockfd, const std::string &input) {
	//check for files existence
	std::string file = input.substr(4, input.length());

	// check if the file already exists in current directory
	FILE* fp = fopen(file.c_str(), "r");

	if(fp) {
		fclose(fp);
		throw "File already exists in current directory, won't overwrite.\n";
	}

	if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
		throw "Failed to send the file for 'get' to the server.";

	// get cid
	if(recv(sockfd, &cid, sizeof(cid), 0) == -1)
		throw "Failed to receive data from the server.";

	std::cout << "Get executed with cid of " << cid << "\n";

	// check if file exists on server
	char fileMessage[100];
	if(recv(sockfd, fileMessage, sizeof(fileMessage), 0) == -1)
		throw "Failed to receive data from the server.";

	if(strcmp(fileMessage, "file exists") != 0)
		throw "File does not exist on the server.";

	// create file on local computer
	int fd = open(file.c_str(), O_WRONLY | O_CREAT, 0666);
	if(fd == -1)
		throw "Failed to open file.";

	// get fileSize from server
	int fileSize;
	if(recv(sockfd, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to receive data from the server.";
	
	//keep reciving data until there's none left
	//keep reciving data until there's none left
	int bytesLeft = fileSize;
	int bytesReceived;
	char output[BUFFSIZE];

	while(bytesLeft > 0) {
		memset(output, 0, BUFFSIZE);

		if((bytesReceived = recv(sockfd, output, BUFFSIZE, 0)) == -1)
			throw "Failed to receive data from the client.\n";
		if(bytesReceived > 0) {
			if(write(fd, output, bytesReceived) != bytesReceived)
				throw "Write error to file.\n";
		}

		bytesLeft -= bytesReceived;
	}

	if(close(fd) == -1)
		throw "Failed to close the file.";
}

//upload file to server
void handlePutCommand(const int &sockfd, const std::string &input) {
	std::string fileName = input.substr(4, input.length());

	// check to make sure file exists on the computer					
	FILE* fp = fopen(fileName.c_str(), "r");

	if(fp == NULL)
		throw "File does not exist in current directory.\n";

	fclose(fp);
	//send the file name
	if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
		throw "Failed to send the file name to the server.";

	// get cid
	if(recv(sockfd, &cid, sizeof(cid), 0) == -1)
		throw "Failed to receive data from the server.";

	std::cout << "Put executed with cid of " << cid << "\n";

	// check to make sure file dosen't exist on server
	char fileMessage[100];
	if(recv(sockfd,	fileMessage, sizeof(fileMessage), 0) == -1)
		throw "Failed to receive data from the server.";

	if(strcmp(fileMessage, "file does not exist") != 0)
		throw "File already exists on the server.";

	int fd;
	if((fd = open(fileName.c_str(), O_RDONLY)) == -1)
			throw "Failed to open file.";

	struct stat sb;
	fstat(fd, &sb);

	//send file size to server
	int fileSize = sb.st_size;
	if(send(sockfd, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to send files metadata.";

	int bytesSent = 0;
	char buffer[BUFFSIZE];

	while(bytesSent < fileSize) {
		int bytesRead = read(fd, buffer, BUFFSIZE);

		if(send(sockfd, buffer, bytesRead, 0) == -1)
			throw "Failed to send 'ls' message to client.\n";

		bytesSent += bytesRead;
		sleep(1);
	}

	if(close(fd) == -1)
		throw "Failed to close the file.";
}

void handler(int num) {
	char signal_message[] = "quit_signal";
	send(sockfd, signal_message, sizeof(signal_message), 0);

	printf("\n");
	exit(EXIT_FAILURE);
}
