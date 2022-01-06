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

