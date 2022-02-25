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
void handleGetCommand(int sockfd, const std::string &input) {
	//check for files existence
	std::string file = input.substr(4, input.length());

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
	
	//keep receiving data until there's none left
	char output[BUFFSIZE];

	ssize_t numRead;
	while((numRead = recv(sockfd, output, BUFFSIZE, 0)) > 0) {
		if(write(fd, output, numRead) != numRead)
			throw "Failed to write data to the file.";
	}

	if(close(fd) == -1)
		throw "Failed to close the file.";
}

//upload file to server
void handlePutCommand(const int &sockfd, const std::string &input) {
	done_sending = false;
	int fd;
	std::string fileName = input.substr(4, input.length());
				
	if((fd = open(fileName.c_str(), O_RDONLY)) == -1)
		throw "Failed to open file.";

	//send the file name
	if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
		throw "Failed to send the file name to the server.";

	// get cid
	if(recv(sockfd, &cid, sizeof(cid), 0) == -1)
		throw "Failed to receive data from the server.";

	std::cout << "Put executed with cid of " << cid << "\n";

	char buffer[BUFFSIZE];
	ssize_t numRead;
	//put data into the socket
	while((numRead = read(fd, buffer, BUFFSIZE)) > 0) {
		if(write(sockfd, buffer, numRead) != numRead)
			throw "Failed to send 'get' content to client.\n";
	}
	
	if(close(fd) == -1)
		throw "Failed to close the file.";

	done_sending = true;
}

//signal handler
void handler(int num) {
	char signal_message[] = "quit_signal";
	send(sockfd, signal_message, sizeof(signal_message), 0);

	printf("\n");
	exit(EXIT_FAILURE);
}
