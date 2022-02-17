#include "myftp.h"
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <exception>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>

//read the current directory
void listDirectories(const int &client_sock) {
	FILE *fd = popen("ls", "r");

	if(fd == NULL)
		throw "Failed to execute 'ls' on the command-line.";

	char buffer[BUFFSIZE];
	while(fgets(buffer, BUFFSIZE, fd) != NULL) {
		if(send(client_sock, buffer, BUFFSIZE, 0) == -1)
			throw "Failed to send 'ls' message to client.\n";

		memset(buffer, '\0', sizeof(buffer));
	}

	fclose(fd);

	char temp[] = "";
	if(send(client_sock, temp, sizeof(temp), 0) == -1)
		throw "Failed to send 'ls' message to client.\n";
}

//send file to the client
void getFile(const std::string &file, const int &client_sock) {
  //background thread
  int fd;
  char ampersand = file[file.length()-1];
  if(ampersand == '&') {
    std::future<bool> backgroundThread = std::async(std::launch::async, boolGet,file, client_sock);
    std::future_status status;
    // Our main program loop
    while(true){
      //Not finished yet
      std::cout << "Main thread is running" << std::endl;
      // artificial sleep for our program
      // allows enough time to test
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      status = backgroundThread.wait_for(std::chrono::milliseconds(1));
      // If our data is ready, that is, our background
      // thread has completed
      if(status == std::future_status::ready){
        std::cout << "Our get file is ready..." << std::endl;
        break;
      }
    }
  } else 
	// check if file exists on the server
	if((fd = open(file.c_str(), O_RDONLY)) == -1) {
		char errorMsg[] = "File does not exist.\n";

		if(send(client_sock, errorMsg, sizeof(errorMsg), 0) == -1)
			throw "Failed to send error msg to client.\n";
	
		return;
	}

	char successMsg[] = "file exists";
	if(send(client_sock, successMsg, sizeof(successMsg), 0) == -1)
			throw "Failed to send error msg to client.\n";

	struct stat sb;
	fstat(fd, &sb);
	// send file size
	int fileSize = sb.st_size;
	if(send(client_sock, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to send the file size.\n";

	if(sendfile(client_sock, fd, 0, sb.st_size) == -1)
		throw "Failed to send file contents.\n";
	
	if(close(fd) == -1)
		throw "Failed to close the file.\n";
}
bool boolGet(const std::string &file, const int &client_sock) {
  int fd;
  if((fd = open(file.c_str(), O_RDONLY)) == -1) {
		char errorMsg[] = "File does not exist.\n";

		if(send(client_sock, errorMsg, sizeof(errorMsg), 0) == -1)
			throw "Failed to send error msg to client.\n";
	
		return true;
	}

	char successMsg[] = "file exists";
	if(send(client_sock, successMsg, sizeof(successMsg), 0) == -1)
			throw "Failed to send error msg to client.\n";

	struct stat sb;
	fstat(fd, &sb);
	// send file size
	int fileSize = sb.st_size;
	if(send(client_sock, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to send the file size.\n";

	if(sendfile(client_sock, fd, 0, sb.st_size) == -1)
		throw "Failed to send file contents.\n";
	
	if(close(fd) == -1)
		throw "Failed to close the file.\n";
  return true;
}
void putFile(const std::string &filename, const int &client_sock) {
	int fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0666);
	char output[BUFFSIZE];
  char ampersand = filename[filename.length()-1];
  if(ampersand == '&') {
    std::future<bool> backgroundThread = std::async(std::launch::async, boolPut,filename, client_sock);
    std::future_status status;
    // Our main program loop
    while(true){
      //Not finished yet
      std::cout << "Main thread is running" << std::endl;
      // artificial sleep for our program
      // allows enough time to test
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
      status = backgroundThread.wait_for(std::chrono::milliseconds(1));
      // If our data is ready, that is, our background
      // thread has completed
      if(status == std::future_status::ready){
        std::cout << "Our get file is ready..." << std::endl;
        break;
      }
    }
  } else 

	if(fd == -1) 
		throw "Failed to open file.\n";

	// get file Size from server
	int fileSize;
	if(recv(client_sock, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to receive data from the client.\n";
	
	//keep reciving data until there's none left
	int bytesLeft = fileSize;
	int bytesRecieved;

	while(bytesLeft > 0) {
		memset(output, 0, BUFFSIZE);

		if((bytesRecieved = recv(client_sock, output, BUFFSIZE, 0)) == -1)
			throw "Failed to receive data from the client.\n";
		if(write(fd, output, bytesRecieved) != bytesRecieved)
			throw "Write error to file.\n";

		bytesLeft -= bytesRecieved;
	}

	if(close(fd) == -1)
		throw "Failed to close the file.\n";
}
bool boolPut(const std::string &filename, const int &client_sock){
  int fd = open(filename.c_str(), O_WRONLY | O_CREAT, 0666);
	char output[BUFFSIZE];

	if(fd == -1) 
		throw "Failed to open file.\n";

	// get file Size from server
	int fileSize;
	if(recv(client_sock, &fileSize, sizeof(int), 0) == -1)
		throw "Failed to receive data from the client.\n";
	
	//keep reciving data until there's none left
	int bytesLeft = fileSize;
	int bytesRecieved;

	while(bytesLeft > 0) {
		memset(output, 0, BUFFSIZE);

		if((bytesRecieved = recv(client_sock, output, BUFFSIZE, 0)) == -1)
			throw "Failed to receive data from the client.\n";
		if(write(fd, output, bytesRecieved) != bytesRecieved)
			throw "Write error to file.\n";

		bytesLeft -= bytesRecieved;
	}

	if(close(fd) == -1)
		throw "Failed to close the file.\n";
  
  return true;
}