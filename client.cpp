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
	std::string input, prevInput;

	if((connect(sockfd, (struct sockaddr*) &addr, sizeof(addr))) == -1)
		errexit("Could not connect to nport.");

	addr.sin_port = htons(t_port_num);//network byte ordering of port number		
	if((connect(sockfd2, (struct sockaddr*) &addr, sizeof(addr))) == -1)
		errexit("Could not connect to tport.");

	//define signal handler for the kill signal(Cntl-C)
	signal(SIGINT, handler);//handler is a function pointer

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
							strcpy(temp, input.c_str());

							int status = pthread_create(&tid, NULL, handleGetBackground, temp);

							if(status != 0) {
								throw "Unable to create thread.\n";
							}
							usleep(100000); // give time for prompt to get to the next line
						} 
						else
							handleGetCommand(sockfd, input);

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
							errexit("Failed to close the socket.");

						return EXIT_SUCCESS;
					}

					case 5: { //ls
						int recv_size;
						
						if(send(sockfd, input.c_str(), BUFFSIZE, 0) == -1)
							throw "Failed to send the input to the server.";

						while(true) {
							memset(output, '\0', BUFFSIZE);

							if((recv_size = recv(sockfd, output, BUFFSIZE, 0)) > 1)
								std::cout << output;
							else
								break;
						}

						std::cout << '\n';
						break;
					}	

					case 9: { // terminate
						// send terminate command to tport
						std::string cidString = input.substr(10, input.length());
						unsigned int cidInput = atoi(cidString.c_str());
						std::cout << "Terminating...\n\n";

						// send terminate command to server
						if(send(sockfd2, &cidInput, sizeof(cidInput), 0) == -1)
							throw "Failed to send the cid to the server.";

						std::string commandType;

						for(unsigned int i = 0; prevInput[i] != ' ' && i < prevInput.length(); ++i)
							commandType += prevInput[i];

						if(commandType.compare("get") == 0) {
							// clean up get/put command
							pthread_cancel(tid);
							std::string prevFile = prevInput.substr(4, input.length() - 2);

							// clear buffer
							sleep(1); // need time to wait for server to stop sending to empty the buffer
							fcntl(sockfd, F_SETFL, O_NONBLOCK); // set socket to non blocking

							for(int i = 0; i < 100; i++)
								recv(sockfd, output, BUFFSIZE, 0);
							
							// set socket back to blocking
							int oldfl = fcntl(sockfd, F_GETFL);
							fcntl(sockfd, F_SETFL, oldfl & ~O_NONBLOCK); // unset

							// remove file
							remove(prevFile.c_str());
						} 
						else {
							sleep(1); // need to wait one second on put to make sure the server dosen't get blocked on reading
							pthread_cancel(tid);
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
