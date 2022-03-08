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
#include <pthread.h>

#define BUFFSIZE 1000
//socket file descriptor
//needs to be global so signal handler can access it
int sockfd, sockfd2;
unsigned int cid;
pthread_t tid;
bool done_sending = false;

//string to int mappings
std::unordered_map<std::string, int> code = {
	{"quit", 1}, {"get", 2}, {"put", 3}, {"delete", 4}, {"ls", 5}, 
	{"pwd", 6}, {"cd", 7}, {"mkdir", 8}, {"terminate", 9} };

//function prototypes
void errexit(const std::string message);
void handleGetCommand(const int &sockfd, const std::string &input);
void handlePutCommand(const int &sockfd, const std::string &input);
void *handleGetBackground(void* arg);
void *handlePutBackground(void* arg);

//signal handler
void handler(int num) {
	char signal_message[] = "quit_signal";
	send(sockfd, signal_message, sizeof(signal_message), 0);

	printf("\n");
	exit(1);
}

int main(int argc, char **argv) {
	if(argc != 4)
		errexit("Format: ./client {server_domain_name} {nport} {tport}\n");

	//command-line argument information
	uint port_num = atoi(argv[2]);
	uint t_port_num = atoi(argv[3]);
	char *domain = argv[1];

	// get ip-address of the domain
	struct hostent *hostInfo;
	struct in_addr **addrList;

	if((hostInfo = gethostbyname(domain)) == NULL)
		errexit("Domain name not found.");

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		errexit("Failed to create a socket.");

	if((sockfd2 = socket(AF_INET, SOCK_STREAM, 0)) == -1)
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
		errexit("Could not connect to nport.");

	addr.sin_port = htons(t_port_num);//network byte ordering of port number		
	if((connect(sockfd2, (struct sockaddr*) &addr, sizeof(addr))) == -1)
		errexit("Could not connect to tport.");

	int option;
	//define signal handler for the kill signal(Cntl-C)
	signal(SIGINT, handler);//handler is a function pointer
	std::string prevInput;

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

				bool isBackground = false;
				if(input[input.length() - 1] == '&') {
					prevInput = input;
					isBackground = true;
				}
        done_sending = false;
				//switch on result
				switch(option) {
					case 2: {//get
						if(isBackground) {
							char temp[BUFFSIZE];
							strcpy(temp, input.c_str());
							int status = pthread_create(&tid, NULL, handleGetBackground, temp);
							if(status != 0) {
								throw "Unable to create thread.\n";
							}
							usleep(100000); // give time for prompt to get to the next line
						} else {
							handleGetCommand(sockfd, input);
						}
						break;
					}
					case 3: {//put
						if(isBackground) {
							char temp[BUFFSIZE];
							strcpy(temp, input.c_str());
							int status = pthread_create(&tid, NULL, handlePutBackground, temp);
							if(status != 0) {
								throw "Unable to create thread.\n";
							}
							usleep(100000); // give time for prompt to get to the next line
						} else {
							handlePutCommand(sockfd, input);
						}
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
					  if(send(sockfd, input.c_str(), input.length(), 0) == -1) 
               throw "Cannot send the command to server";
            if(recv(sockfd, output, BUFFSIZE, 0) == -1)
							throw "Failed to receive data from the server.";
            std::cout << output;
						std::cout << '\n';
            memset(output, '\0', BUFFSIZE);
						break;
					}	

					case 9: { // terminate
						// send terminate command to tport
						std::string cidString = input.substr(10, input.length() - 10);
						unsigned int cidInput = atoi(cidString.c_str());
						std::cout <<"Terminating...\n\n";

						std::string commandType = prevInput.substr(0,3);
						if(commandType.compare("get") == 0) {
						  // send terminate command to server
  						if(send(sockfd2, &cidInput, sizeof(cidInput), 0) == -1)
							  throw "Failed to send the cid to the server.";
                                        
							// clean up get/put command
							pthread_cancel(tid);
							std::string prevFile = prevInput.substr(4, input.length() - 2);

							// clear buffer
							sleep(1); // need time to wait for server to stop sending to empty the buffer
							fcntl(sockfd, F_SETFL, O_NONBLOCK); // set socket to non blocking
							for(int i = 0; i < 1000; i++) {
								recv(sockfd, output, BUFFSIZE, 0);
							}
							// set socket back to blocking
							int oldfl = fcntl(sockfd, F_GETFL);
							fcntl(sockfd, F_SETFL, oldfl & ~O_NONBLOCK); // unset

							// remove file
							remove(prevFile.c_str());
						} else {
							done_sending = true;
              sleep(1);
					  	// send terminate command to server
						  if(send(sockfd2, &cidInput, sizeof(cidInput), 0) == -1)
							  throw "Failed to send the cid to the server.";
						  }
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

void *handleGetBackground(void* arg) {
	try {
		char *temp = (char*)arg;
		std::string inputText(temp);
		inputText = inputText.substr(0, inputText.length() -2);
		handleGetCommand(sockfd, inputText);
	} catch(const char* hello) {
		std::cerr << hello << "\n";
	}
	pthread_exit(0);
}

void *handlePutBackground(void* arg) {
	try {
		char *temp = (char*)arg;
		std::string inputText(temp);
		inputText = inputText.substr(0, inputText.length() -2);
		handlePutCommand(sockfd, inputText);
	} catch(const char* hello) {
		std::cerr << hello << "\n";
	}
	pthread_exit(0);
}

//download file from server
void handleGetCommand(const int &sockfd, const std::string &input) {
	//check for files existence
	std::string file = input.substr(4, input.length());


  // send command to server
	if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
		throw "Failed to send the file for 'get' to the server.";

	// get cid
	if(recv(sockfd, &cid, sizeof(cid), 0) == -1)
		throw "Failed to receive data from the server.";
	std::cout << "Get executed with cid of " << cid << "\n\n";

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
	ssize_t bytesReceived = 0;
  ssize_t bytesLeft = fileSize;
	char output[BUFFSIZE];

	while(bytesLeft > 0) {
		memset(output, '\0', BUFFSIZE);

		if((bytesReceived = recv(sockfd, output, BUFFSIZE, 0)) == -1)
			 throw "Failed to receive data from the server.";

    if(bytesReceived > 0) {
		  if(write(fd, output, bytesReceived) != bytesReceived)
			  throw "Write error to file.";
    }
    
		bytesLeft -= bytesReceived;
   
    if(bytesLeft == 0) {
      break;
    }
	}

	if(close(fd) == -1) {
		throw "Failed to close the file.";
  }
}

//upload file to server
void handlePutCommand(const int &sockfd, const std::string &input) {
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

	// get cid
	if(recv(sockfd, &cid, sizeof(cid), 0) == -1)
		throw "Failed to receive data from the server.";
	std::cout << "Put executed with cid of " << cid << "\n\n";

	int fd;
	if((fd = open(fileName.c_str(), O_RDONLY)) == -1)
			throw "Failed to open file.";

	struct stat sb;
	fstat(fd, &sb);

	//send file size to server
	int fileSize = sb.st_size;
	if(send(sockfd, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to send file's metadata.";

	int bytesSent = 0;
	char buffer[BUFFSIZE];
	while(bytesSent < fileSize) {
		int bytesRead = read(fd, buffer, BUFFSIZE);
		if(send(sockfd, buffer, bytesRead, 0) == -1)
			throw "Failed to send 'ls' message to client.\n";
    if(done_sending) {
      break;
    }

		bytesSent += bytesRead;
		usleep(10);
	}
	

/*
	if(sendfile(sockfd, fd, 0, sb.st_size) == -1)
		throw "Failed to send file's contents.";
*/

	if(close(fd) == -1)
		throw "Failed to close the file.";
}

