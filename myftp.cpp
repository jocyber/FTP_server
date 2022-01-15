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

