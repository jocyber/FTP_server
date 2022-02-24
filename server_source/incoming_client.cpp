#include "client_handlers.h"

//resitrict the number of connections
unsigned int numConnections = 0;

void* connect_client(void* socket);

//map the strings to codes so the strings will work with a switch statement
std::unordered_map<std::string, int> code = {
	{"quit", 1}, {"get", 2}, {"put", 3}, {"delete", 4}, {"ls", 5}, 
	{"pwd", 6}, {"cd", 7}, {"mkdir", 8}, {"quit_signal", 9} };

//handle the connections on the normal port
void* handle_client(void* port) {
    unsigned short nPORT = *(unsigned short*) port;
    int sockfd, client_sock;

	//AF_INET = IPv4
	//SOCK_STREAM = TCP
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
			exitFailure("Failed to create a socket.");

	//initialize and set the structure to all zeros
	struct sockaddr_in addr;
	memset((struct sockaddr_in *) &addr, 0, sizeof(addr));

	addr.sin_family = AF_INET; // IPv4
	addr.sin_port = htons(nPORT);//host to network short/uint16(network byte ordering)
	addr.sin_addr.s_addr = INADDR_ANY;

	if(bind(sockfd, (struct sockaddr *) &addr, sizeof(addr)) == -1)
		exitFailure("Could not bind the socket to an address for nPORT");

	//listen for oncoming connections
	//SOMAXCONN = socket maximum connections
	if(listen(sockfd, SOMAXCONN) == -1)
		exitFailure("Could not set up a passive socket connection.");

	//create a separate thread for each client that connects
	pthread_t tid;
	
	while(true) {//on-demand model
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
		int	thStatus = pthread_create(&tid, NULL, connect_client, &client_sock);
		numConnections++;

		if(thStatus != 0)
			exitFailure("Failed to create thread.");
	}
}

//function to handle an individual client
void* connect_client(void *socket) {
	int client_sock = *(int *) socket;
	char buffer[BUFFSIZE];
	char message[BUFFSIZE];
	bool stop = false; // flag for exit
	unsigned int tempcid;

	while(true) {
		//handle exceptions by throwing messages and network errors
		try {
			//receive input from the client
			if((recv(client_sock, buffer, BUFFSIZE, 0)) == -1)
				throw "Failed to receive data from the client.";

			std::string command, client_input(buffer);
			unsigned int i = 0;

			//extract command
			for(; buffer[i] != ' ' && i < client_input.length(); ++i)
				command += client_input[i];
	
			//client_input now becomes the argument
			if(client_input[client_input.length() - 1] == '&') {
				client_input = client_input.substr(i + 1, client_input.length() - 3 - i);
			} else {
				if(command.length() != client_input.length())
					client_input = client_input.substr(i + 1, client_input.length());	
			}

			int option = -1;

			if(code.find(command) != code.end())// if command is valid
				option = code[command];

			//general logic for the ftp server starts here
			//compare the client input to different options.
			switch(option) {
				case 5: {// ls
					listDirectories(client_sock);
					break;
				}

				case 7: // cd
					//change the directory
					if(chdir(client_input.c_str()) == -1) {
						std::string error;
						error = "No directory named {" + client_input + "}";

						if(send(client_sock, error.c_str(), BUFFSIZE, 0) == -1)
						 throw "Failed to send error message for 'cd' to client.\n";	
					}
					else {
						if(send(client_sock, message, BUFFSIZE, 0) == -1) //send an empty message to indicate success
							throw "Failed to send success message to client from 'cd'.\n";
					}

					break;

				case 6: //pwd
					char *result;

					if((result = getcwd(message, BUFFSIZE)) == NULL) {
						char response[] = "Path name too long.";
						if(send(client_sock, response, BUFFSIZE, 0) == -1)
							throw "Failed to send 'pwd' error response to client.\n";
					}
					else {
						if(send(client_sock, result, BUFFSIZE, 0) == -1)
							throw "Failed to send 'pwd' response to client.\n";
					}

					break;

				case 1: case 9://quit or quit_signal
					//close the socket when the client has left the active session
					if(close(client_sock) == -1)
						throw Network_Error("Could not close the active socket connection.\n");

					if(numConnections > 0)
						numConnections--;

					stop = true; // set flag to true and end ftp session
					break;

				case 4: {// delete
					//remove file or empty directory
					int fd = open(client_input.c_str(), O_RDONLY);

					if(fd == -1) {
						client_input = "File: {" + client_input + "} does not exist.\n";
						
						if(send(client_sock, client_input.c_str(), client_input.length(), 0) == -1) {
							close(fd);
							throw "Failed to send error message for 'delete'.\n";
						}
					}
					else {
						if(flock(fd, LOCK_EX) == -1) {
							close(fd);
							throw "Failed to place lock on file descriptor for 'delete' command.";
						}

						//remove the file
						remove(const_cast<char*>(client_input.c_str()));

						//send successfull packet back to the client
						if(send(client_sock, message, BUFFSIZE, 0) == -1) {
							close(fd);
							throw "Failed to send 'delete' response to client.\n";
						}

						if(flock(fd, LOCK_UN) == -1) {
							close(fd);
							throw "Failed unlock the file for 'delete' command.";
						}
					}

					close(fd);

					break;
				}

				case 8://mkdir
					//make directory with read and write permissions
					mode_t flags;
					flags = S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;

					if(mkdir(const_cast<char*>(client_input.c_str()), flags) == -1) {
						client_input = "Directory {" + client_input + "} already exists.\n";

						if(send(client_sock, client_input.c_str(), client_input.length(), 0) == -1)
							throw "Failed to send 'mkdir' error message.\n";
					}
					else
						if(send(client_sock, message, BUFFSIZE, 0) == -1) // send empty string to indicate success
							throw "Failed to send 'mkdir' response to client.\n";

					break;

				case 2://get
					//get command id
					//avoid deadlock situation
					while(pthread_mutex_trylock(&commandID_lock) == EBUSY) {;}
					tempcid = commandID;
					commandID++;
					pthread_mutex_unlock(&commandID_lock);
					
					// add to hash table
					while(pthread_mutex_trylock(&hashTableLock) == EBUSY) {;}
					globalTable[tempcid] = false;
					pthread_mutex_unlock(&hashTableLock);

					// send command id to client
					if(send(client_sock, &tempcid, sizeof(tempcid), 0) == -1)
						throw "Failed to send error msg to client.\n";

					// upload file to client
					getFile(client_input, client_sock, tempcid);

					// remove from hash table
					while(pthread_mutex_trylock(&hashTableLock) == EBUSY) {;}
					globalTable.erase(tempcid);
					pthread_mutex_unlock(&hashTableLock);

					break;

				case 3://put
					// get command id
					while(pthread_mutex_trylock(&commandID_lock) == EBUSY) {;}
					tempcid = commandID;
					commandID++;
					pthread_mutex_unlock(&commandID_lock);
					
					// add to hash table
					while(pthread_mutex_trylock(&hashTableLock) == EBUSY) {;}
					globalTable[tempcid] = false;
					pthread_mutex_unlock(&hashTableLock);

					// send command id to client
					if(send(client_sock, &tempcid, sizeof(tempcid), 0) == -1)
						throw "Failed to send error msg to client.\n";

					//download file from client
					putFile(client_input, client_sock, tempcid);

					// remove from hash table
					while(pthread_mutex_trylock(&hashTableLock) == EBUSY) {;}
					globalTable.erase(tempcid);
					pthread_mutex_unlock(&hashTableLock);
					break;

				default:
					strcpy(message, "Input not recognized.");
					if(send(client_sock, message, BUFFSIZE, 0) == -1)
						throw "Failed to send 'input not recognized' to client.\n";
			}
		}
		catch(const char *message) {
			std::cerr << message;
		}
		catch(Network_Error &message) {//for catastrophic network errors
			std::cerr << message.what();
			pthread_exit((void*)EXIT_FAILURE);
		}
		catch(...) {
			std::cerr << "Unknown error occured.\n";
		}

		//if pthread_exit is called within a try block, it causes a fatal error
		//for some reason
		if(stop)
			pthread_exit(EXIT_SUCCESS);

		memset(message, 0, BUFFSIZE);
		memset(buffer, 0, BUFFSIZE);
	}
}