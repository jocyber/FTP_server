#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include "myftp.h"

using uint = unsigned int;
#define PORT 2000
#define BUFFSIZE 512
#define IP "192.168.1.100"
unsigned int numConnections = 0;

void exitFailure(const std::string str) {
	std::cout << str << '\n';
	exit(EXIT_FAILURE);
}

//function pointer to main logic
void* handleClient(void *socket);

int main(int argc, char **argv) 
{
	int sockfd, client_sock;

	//AF_INET = IPv4
	//SOCK_STREAM = TCP
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			exitFailure("Failed to create a socket.");

	//initialize and set the structure to all zeros
	struct sockaddr_in addr;
	memset((struct sockaddr_in *) &addr, 0, sizeof(addr));

	addr.sin_family = AF_INET; // IPv4
	addr.sin_port = htons(PORT);//host to network short/uint16(network byte ordering)
	addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		exitFailure("Could not bind the socket to an address.");

	//listen for oncoming connections
	//SOMAXCONN = socket maximum connections
	if(listen(sockfd, SOMAXCONN) == -1)
		exitFailure("Could not set up a passive socket connection.");

	//create a separate thread for each client that connects
	pthread_t tid;
	
	while(true) {
		//accept oncoming connection
		if((client_sock = accept(sockfd, NULL, NULL)) == -1)
			exitFailure("Failed to accept client connection.");

		//if max number of connections is reached
		if(numConnections >= SOMAXCONN) {
			//send a message to the client
			if(close(client_sock) == -1)
				exitFailure("Failed to close the client socket");

			continue; //jump back to the beginnning of the loop
		}

		//create a thread and pass the handleClient function pointer and its parameter
		//later, we should implement a thread pool for better performance
		int	thStatus = pthread_create(&tid, NULL, handleClient, &client_sock);
		numConnections++;

		if(thStatus != 0)
			exitFailure("Failed to create thread.");
	}
}

//function to handle an individual client
void* handleClient(void *socket) {
	int client_sock = *(int *) socket;
	char buffer[BUFFSIZE];
	char message[BUFFSIZE];

	while(true) {
		//receive input from the client
		if((recv(client_sock, buffer, BUFFSIZE, 0)) == -1)
			exitFailure("Failed to receive data from the client.");

		std::string client_input(buffer);	

		//general logic for the ftp server starts here
		//compare the client input to different options.
		if(client_input.compare("ls") == 0) {
			listDirectories(message);
			send(client_sock, message, BUFFSIZE, 0);
		}
		else if(client_input.substr(0, 2).compare("cd") == 0) {
			//buffer without cd
			std::string path = client_input.substr(3, client_input.length());	

			//change the directory
			if(chdir(path.c_str()) == -1) {
				std::string error = "No directory named {" + path + "}\n";
				send(client_sock, error.c_str(), BUFFSIZE, 0); 
			}
			else
				send(client_sock, message, BUFFSIZE, 0);//send an empty message to indicate success
		}
		else if(client_input.compare("pwd") == 0) { //print working directory
				char *result;

				if((result = getcwd(message, BUFFSIZE)) == NULL) {
					char response[] = "Path name too long.";
					send(client_sock, response, BUFFSIZE, 0);
				}
				else {
					send(client_sock, result, BUFFSIZE, 0);
				}
		}
		else if(strcmp(buffer, "quit") == 0) {
			strcpy(message, "quit");
			send(client_sock, message, BUFFSIZE, 0);

			//close the socket when the client has left the active session
			if(close(client_sock) == -1)
				exitFailure("Could not close the active socket connection.");

			if(numConnections > 0)
				numConnections--;

			pthread_exit(EXIT_SUCCESS);
		}
		else if(client_input.substr(0, 6).compare("delete") == 0) {
			std::string file = client_input.substr(7, client_input.length());

			if(remove(const_cast<char*>(file.c_str())) == -1) {
			 	file = "File: {" + file + "} does not exist.\n";
				send(client_sock, file.c_str(), file.length(), 0);
			}
			else
				send(client_sock, message, BUFFSIZE, 0);
		}
		else if(client_input.substr(0, 5).compare("mkdir") == 0) {
			std::string directory = client_input.substr(6, client_input.length());

			//make directory with read and write permissions
			if(mkdir(const_cast<char*>(directory.c_str()), S_IRUSR | S_IWUSR) == -1) {
				directory = "Directory {" + directory + "} already exists.\n";
				send(client_sock, directory.c_str(), directory.length(), 0);
			}
			else
				send(client_sock, message, BUFFSIZE, 0);
		}
		else if(client_input.substr(0,3).compare("get") == 0) {
			//endline to make parsing the packet easier
			const std::string file = client_input.substr(4, client_input.length());

			if(getFile(file, client_sock) == -1) {
				std::string error = "Failed to send the requested file.\n";
				send(client_sock, error.c_str(), error.length(), 0);
			}
		}
		else {
			strcpy(message, "Input not recognized.");
			send(client_sock, message, BUFFSIZE, 0);
		}

		//reset buffers - O(1)
		message[0] = '\0';
		buffer[0] = '\0';
	}
}

