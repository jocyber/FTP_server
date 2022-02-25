#include "client_functions.h"

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

	if((terminate_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		errexit("Failed to create a socket.");

	struct sockaddr_in addr;
	memset((struct sockaddr*) &addr, 0, sizeof(addr));
	addrList = (struct in_addr **)hostInfo->h_addr_list;

	char domainIP[100];
	strcpy(domainIP, inet_ntoa(*addrList[0]));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(port_num);//network byte ordering of port number
	addr.sin_addr.s_addr = inet_addr(domainIP);

	std::string input, prevInput;

	if((connect(sockfd, (struct sockaddr*) &addr, sizeof(addr))) == -1)
		errexit("Could not connect to nport.");

	addr.sin_port = htons(t_port_num);//network byte ordering of port number		
	if((connect(terminate_sock, (struct sockaddr*) &addr, sizeof(addr))) == -1)
		errexit("Could not connect to tport.");

	//define signal handler for the kill signal(Cntl-C)
	signal(SIGINT, handler);//handler is a function pointer

	while(true) {
		try {
			input.clear();//O(1)

			//clear out socket
			char tempBuff[BUFFSIZE];
			memset(tempBuff, '\0', BUFFSIZE);
			//non-blocking recieve to clear out remaining data in socket

			fcntl(sockfd, F_SETFL, O_NONBLOCK); // set socket to non blocking
			for(int i = 0; i < 5; i++) {
				//sleep(1);
				recv(sockfd, tempBuff, BUFFSIZE, 0);
			}
		
			// set socket back to blocking
			int oldfl = fcntl(sockfd, F_GETFL);
			fcntl(sockfd, F_SETFL, oldfl & ~O_NONBLOCK); // unset

			std::cout << "myftp> ";

			std::getline(std::cin, input);
			//done_sending = false;

			//guard against buffer overflow
			if(input.length() > BUFFSIZE)
				throw "Buffer overflow error.";
			else {
				//get the command type
				std::string req = "";

				for(unsigned int i = 0; input[i] != ' ' && i < input.length(); ++i)
					req += input[i];

				int option = -1;

				if(code.find(req) != code.end())
					option = code[req];

				bool isBackground = false;
				if(input[input.length() - 1] == '&') {
					prevInput = input;
					isBackground = true;
				}

				//switch on result
				switch(option) {
					case 2: {//get
						if(isBackground) {
							char temp[BUFFSIZE];
							memset(temp, '\0', BUFFSIZE);
							
							strcpy(temp, input.c_str());

							int status = pthread_create(&tid, NULL, handleGetBackground, temp);

							if(status != 0)
								throw "Unable to create thread.\n";
							
							usleep(100000); // give time for prompt to get to the next line
						} 
						else
							handleGetCommand(sockfd, input);

						break;
					}
					case 3: {//put
						if(isBackground) {
							char temp[BUFFSIZE];
							memset(temp, '\0', BUFFSIZE);

							strcpy(temp, input.c_str());

							int status = pthread_create(&tid, NULL, handlePutBackground, temp);

							if(status != 0) {
								throw "Unable to create thread.\n";
							}
							usleep(100000); // give time for prompt to get to the next line
						} 
						else
							handlePutCommand(sockfd, input);
						
						break;
					}
					case 1: {//quit
						char quit_mess[] = "quit";
						
						if(send(sockfd, quit_mess, sizeof(quit_mess), 0) == -1)
							throw "Failed to send the input to the server.";

						if(close(sockfd) == -1)
							errexit("Failed to close the normal socket.");
						
						if(close(terminate_sock) == -1)
							errexit("Failed to close the terminate socket.");

						return EXIT_SUCCESS;
					}

					case 5: { //ls
						if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
							throw "Failed to send the input to the server.";

						char output_mess[BUFFSIZE];
						memset(output_mess, '\0', BUFFSIZE);

						ssize_t numRead;
						while((numRead = recv(sockfd, output_mess, BUFFSIZE, 0)) > 0) {
							std::cout << output_mess;

							if(numRead < BUFFSIZE) {
								done_sending = false;
								break;
							}
						}

						std::cout << '\n';
						break;
					}	

					case 9: { // terminate
						// send terminate command to tport
						if(!isDone) {
							int cidInput = (int)(input[input.length() - 1] - '0');
							std::cout << "Terminating...\n\n";

							// send terminate command to server
							if(send(terminate_sock, &cidInput, sizeof(cidInput), 0) == -1)
								throw "Failed to send the cid to the server.";

							char ACK[4];
							memset(ACK, '\0', sizeof(ACK));

							if(recv(terminate_sock, ACK, sizeof(ACK), 0) == -1)
								throw "Failed to recieve Acknowledgement from the termination port";

							//std::string commandType = prevInput.substr(0, 3);
							done_sending = true;
						}

						break;
					}

					default:
						if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
							throw "Failed to send the input to the server.";

						if(send(terminate_sock, input.c_str(), BUFFSIZE, 0) == -1)
							throw "Failed to send the input to the server.";

						char output[BUFFSIZE];
						memset(output, '\0', BUFFSIZE);

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