#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <bitset>
#include "myftp.h"

using uint = unsigned int;
#define PORT 2000
#define BUFFSIZE 512

void exitFailure(const std::string str) {
	std::cout << str << '\n';
	exit(EXIT_FAILURE);
}

void* handleClient(void *socket);

int main(int argc, char **argv) 
{
	int sockfd, client_sock;
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

	//create a separate thread for each client that connects
	pthread_t tid[SOMAXCONN];
	std::bitset<SOMAXCONN> bits;

	int i = 0;
	
	while(true) {
		if((client_sock = accept(sockfd, NULL, NULL)) == -1)
			exitFailure("Failed to accept client connection.");

		//if max number of connections is reached or the current thread is active
		if(i >= SOMAXCONN || bits.test(i)) {
			bool flag = false;

			//search for a spot in the thread buffer that is not occupied
			for(unsigned int j = 0; j < SOMAXCONN; ++j) {
				if(!bits.test(j)) {
					i = j; //move current thread position to a spot that is unoccupied
					flag = true;
					break;
				}
			}

			//send a message if all bits are set (i.e none of the threads are available)
			if(!flag)
				continue;
		}

		//create a thread and pass the handleClient function pointer and its parameter
		bits.set(i);
		int	thStatus = pthread_create(&tid[i++], NULL, handleClient, &client_sock);

		if(thStatus != 0)
			exitFailure("Failed to create thread.");

		if(pthread_join(tid[i - 1], NULL) != 0)
			exitFailure("Failed to end thread.");
		else {
			bits.flip(i - 1); //set thread bit to inactive
		}
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

		//general logic for the ftp server starts here
		//compare the client input to different options.
		if(strcmp(buffer, "ls") == 0) {
			listDirectories(message);
			send(client_sock, message, BUFFSIZE, 0);
		}
		else if(strcmp(buffer, "cd") == 0) {
			; //handle change directory
		}
		else if(strcmp(buffer, "exit") == 0) {
			strcpy(message, "exit");
			send(client_sock, message, BUFFSIZE, 0);

			//close the socket when the client has left the active session
			if(close(client_sock) == -1)
				exitFailure("Could not close the active socket connection.");

			pthread_exit(EXIT_SUCCESS);
		}
		else {
			strcpy(message, "Input not recognized.");
			send(client_sock, message, BUFFSIZE, 0);
		}

		//reset buffers
		message[0] = '\0';
		buffer[0] = '\0';
	}
}

