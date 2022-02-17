#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "myftp.h"
#include "client_handlers.h"

//exit the program on critical error
void exitFailure(const std::string str) {
	std::cout << str << '\n';
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv) 
{
	// get port number from command line
	if(argc != 3) {
		std::cerr << "Incorrect format. exe {server port} {terminate port}";
		return 1;
	}

	unsigned short nPORT = atoi(argv[1]); // normal port
	unsigned short tPORT = atoi(argv[2]); //terminate port

	pthread_t tid, tid2;
	int check;
	void* client_status;
	void* terminate_status;

	//normal port execution
	check = pthread_create(&tid, NULL, handle_client, &nPORT);

	if(check != 0) {
		std::cerr << "Failed to create the thread for the client.\n";
		exit(EXIT_FAILURE);
	}
 
	//termination port execution
	check = pthread_create(&tid2, NULL, handle_termination, &tPORT);

	if(check != 0) {
		std::cerr << "Failed to create termination thread.\n";
		exit(EXIT_FAILURE);
	}

	pthread_join(tid, &client_status);//argument used to check if the thread finished successfully (infinite, so never checked)
	pthread_join(tid2, &terminate_status);

	return EXIT_SUCCESS;
}

