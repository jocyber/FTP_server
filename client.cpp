#include <sys/socket.h>
#include <iostream>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define PORT 2000
#define BUFFSIZE 512
using uint = unsigned int;

void errexit(const std::string message);

int main(int argc, char **argv)
{
	int sockfd;

	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		errexit("Failed to create a socket.");

	struct sockaddr_in addr;
	memset((struct sockaddr*) &addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = INADDR_ANY;

	char input[BUFFSIZE] = "";
	char output[BUFFSIZE] = "";

	if((connect(sockfd, (struct sockaddr*) &addr, sizeof(addr))) == -1)
		errexit("Could not connect to the socket.");

	while(true) {
		//empty the string
		input[0] = 0;
		output[0] = 0;
		std::cout << "/directory$ ";

		if(scanf("%s", input) == -1)
			errexit("Unknown problem when reading input.");

		//guard against buffer overflow
		if(sizeof(input) > BUFFSIZE)
			std::cerr << "Buffer overflow.\n";
		else {
			send(sockfd, input, BUFFSIZE, 0);
		
			//print what the server responds with
			if(recv(sockfd, output, BUFFSIZE, 0) == -1)
				errexit("Failed to receive data from the server.");

			if(strcmp(output, "exit") == 0)
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
