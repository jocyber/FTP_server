#include "myftp.h"
#include <netinet/in.h>
#include <netinet/tcp.h>

//read the current directory
void listDirectories(char message[]) {
	//local variables
  char path[] = "./";
	char space[] = " ";

  //pointer reference
	DIR *direct = opendir(path);
	struct dirent *dirP;	

  //while statement
	while((dirP = readdir(direct)) != NULL) {
		if(strcmp(dirP->d_name, ".") == 0 || strcmp(dirP->d_name, "..") == 0)
			continue;
      
    //returns pointer
		strcat(message, dirP->d_name);
		strcat(message, space);
	}
}

//send file to the client
ssize_t getFile(const std::string &file, const int &client_sock) {
	//local variable
  int fd;
  //opens tile
	if((fd = open(file.c_str(), O_RDONLY)) == -1)
			return -1;

  //retrieves and sends file
	struct stat sb;
	stat(file.c_str(), &sb);
	send(client_sock, &sb, sizeof(sb), 0);

	int optval = 1;
	//enable tcp_cork (only send a packet when it's full, meaning network congestion is reduced)
	setsockopt(client_sock, IPPROTO_TCP, TCP_CORK, &optval, sizeof(optval));
	sendfile(client_sock, fd, 0, sb.st_size);
	//disable tcp_cork
	optval = 0;
	setsockopt(client_sock, IPPROTO_TCP, TCP_CORK, &optval, sizeof(optval));
	
 //closes file
	if(close(fd) == -1)
		return -1;

	return 0;
}
