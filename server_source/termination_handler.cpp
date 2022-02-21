#include "client_handlers.h"

//listen for the 'terminate' command
void* handle_termination(void* port) {
    unsigned short tPORT = *(unsigned short*) port;
    //create the socket and binding, then wait for a termination command

    int sockfd;

	//AF_INET = IPv4
	//SOCK_STREAM = TCP
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			exitFailure("Failed to create a socket in termination thread.");

	//initialize and set the structure to all zeros
	struct sockaddr_in addr;
	memset((struct sockaddr_in *) &addr, 0, sizeof(addr));

	addr.sin_family = AF_INET; // IPv4
	addr.sin_port = htons(tPORT);//host to network short/uint16(network byte ordering)
	addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		exitFailure("Could not bind the socket to an address for tPORT.");

	//listen for oncoming connections
	//SOMAXCONN = socket maximum connections
	if(listen(sockfd, SOMAXCONN) == -1)
		exitFailure("Could not set up a passive socket connection in termination thread.");

	unsigned int cid;
	int client_sock;

	if((client_sock = accept(sockfd, NULL, NULL)) == -1)
		std::cerr << "Check shit.\n";

	while(true) {
		// if cid is 0, it will recv 0 when client connects and disconnects, needs to start at 1
		if(recv(client_sock, &cid, sizeof(cid), 0) == 0)
			pthread_exit(EXIT_SUCCESS);

		if(cid > 0) {
			std::cout << "Terminating Process: " << cid << "\n";

			while(pthread_mutex_trylock(&hashTableLock) == EBUSY) {;}
			globalTable[cid] = true;
			pthread_mutex_unlock(&hashTableLock);
		}
	}

    pthread_exit(0); //just here for now to avoid compiler warnings
}