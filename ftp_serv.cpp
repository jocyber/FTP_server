#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include "myftp.h"

using uint = unsigned int;
#define PORT 2000
#define BUFFSIZE 512

void exitFailure(const std::string str) {
	std::cout << str << '\n';
	exit(1);
}

int main(int argc, char **argv) 
{
	int sockfd, client_sock;
	char buffer[BUFFSIZE];
	char message[BUFFSIZE];
	char curr_direct[BUFFSIZE] = "./";

	//AF_INET = IPv4
	//SOCK_STREAM = TCP
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			exitFailure("Failed to create a socket.");

	//reset the structure
	struct sockaddr_in addr;
	memset((struct sockaddr_in *) &addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);//host to network short/uint16(network byte ordering)
	addr.sin_addr.s_addr = INADDR_ANY;//IPv4 'wildcard'

	if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		exitFailure("Could not bind the socket to an address.");

	//listen for oncoming connections
	if(listen(sockfd, SOMAXCONN) == -1)
		exitFailure("Could not set up a passive socket connection.");

	if((client_sock = accept(sockfd, NULL, NULL)) == -1)
		exitFailure("Could not accept active socket connection.");
	
	//server runs forever until an error occurs
	while(true) {

		//receive input from the client
		if((recv(client_sock, buffer, BUFFSIZE, 0)) == -1) {
			//if client disconnected, then accept a new connection
			if((client_sock = accept(sockfd, NULL, NULL)) == -1)
				exitFailure("Could not accept active socket connection.");

			//refill the buffer with the data from the new connection
			if(recv(client_sock, buffer, BUFFSIZE, 0) == -1)
				exitFailure("Failed to receive data from the client.");
		}

		//general logic for the ftp server starts here
		//compare the client input to different options.
		if(strcmp(buffer, "ls") == 0) {
			listDirectories(message);
			send(client_sock, message, BUFFSIZE, 0);
		}
		else if(strcmp(buffer, "cd")) {
			; //handle change directory
		}
		else if(strcmp(buffer, "exit") == 0) {
			strcpy(message, "exit");
			send(client_sock, message, BUFFSIZE, 0);

			//close the socket when the client has left the active session
			if(close(client_sock) == -1)
				exitFailure("Could not close the active socket connection.");
		}
		else {
			strcpy(message, "Input not recognized.");
			send(client_sock, message, BUFFSIZE, 0);
		}

		message[0] = 0;
		buffer[0] = 0;
	}
}
