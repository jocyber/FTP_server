#include "myftp.h"

//read the current directory
void listDirectories(char message[]) {
	char path[] = "./";
	char space[] = " ";

	DIR *direct = opendir(path);
	struct dirent *dirP;	

	while((dirP = readdir(direct)) != NULL) {
		if(strcmp(dirP->d_name, ".") == 0 || strcmp(dirP->d_name, "..") == 0)
			continue;

		strcat(message, dirP->d_name);
		strcat(message, space);
	}
}

// send the file to the client
int getFile(char message[], std::string filename, int client_sock, char buffer[]) {
	// open file, if error return "file not found"
	int file_fd = open(filename.c_str(), O_RDONLY);
	if(file_fd < 0) {
		std::string error = "file not found";
		send(client_sock, error.c_str(), BUFFSIZE, 0); 
		//reset buffers - O(1)
		message[0] = '\0';
		buffer[0] = '\0';
		return -1;
	}
	send(client_sock, "file found", BUFFSIZE, 0); 

	// if file is opened, send file, and permissions
	struct stat fileStat;
	fstat(file_fd, &fileStat);
	send(client_sock, &fileStat, sizeof(fileStat), 0);
	sendfile(client_sock, file_fd, 0, fileStat.st_size);

	// close file
	close(file_fd);
	return 0;
}

int putFile(char message[], std::string filename, int client_sock, char buffer[]) {
	// check if file already exits, if it does return "file found"
	filename = "copied_" + filename;  // used for local testing where files are the same
	int file_fd = open(filename.c_str(), O_RDONLY );
	if( file_fd > 0) {
		std::string error = "file exists";
		send(client_sock, error.c_str(), BUFFSIZE, 0); 
		close(file_fd);
		return -1;
	}
	send(client_sock, "file not found", BUFFSIZE, 0);

	// copy from client to file
	struct stat fileStat;
	if(recv(client_sock, &fileStat, sizeof(fileStat), 0) == -1) {
		std::cout << "Error recieving from client";
		exit(EXIT_FAILURE);
	}

	file_fd = open(filename.c_str(), O_CREAT | O_APPEND | O_WRONLY, 0666 );
	if( file_fd < 0) {
		std::cout << "Error opening file.\n";
		exit(EXIT_FAILURE);
	}

	int bitsLeft = fileStat.st_size;
	int bitsRecieved;
	char output[BUFFSIZE];
	while(bitsLeft > 0) {
		memset(output, 0, BUFFSIZE);
		if((bitsRecieved = recv(client_sock, output, BUFFSIZE, 0)) == -1) {
			std::cout << "Error recieving from client";
			exit(EXIT_FAILURE);
		}
		if(write(file_fd, output, sizeof(char) * bitsRecieved) != bitsRecieved) {
			std::cout << "Write error to file.";
			exit(EXIT_FAILURE);
		}
		bitsLeft -= bitsRecieved;
		output[0] = '\0';
	}

	close(file_fd);

	return 1;
}
