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

int changeDirectory(char path[], char newDirectory[]) {
	if(strcmp(newDirectory, ".") == 0)
		return 0;
	
	DIR *direct = opendir(path);
	struct dirent *dir;

	while((dir = readdir(direct) != NULL) {
		struct stat sb;

		if(stat(path, &sb) == -1)
			continue;

		if(S_ISDIR(sb.st_mode) && strcmp(newDirectory, dir->d_name) == 0) {
			;//change directory
		}
	}
	
	return -1;
}




