#ifndef FTP
#define FTP

#include <iostream>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <exception>
#include <unordered_map>
#include <future>
#include <thread>
#include <chrono>

#define BUFFSIZE 1000

//custom exception for network errors
class Network_Error : std::exception {
	const char *message;

public:
	Network_Error(const char* _message) : message(_message) {}

	//override virtual 'what()' member function
	const char* what() const throw() {
		return message;
	}
};

void listDirectories(const int &client_sock);
void getFile(const std::string &file, const int &client_sock);
void putFile(const std::string &filename, const int &client_sock);
bool boolGet(const std::string &file, const int &client_sock);
bool boolPut(const std::string &filename, const int &client_sock);

#endif
