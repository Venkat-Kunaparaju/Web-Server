#include <stdio.h>
#include<stdlib.h>


#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>




//Constants
#define PORT 1500
int main() {

    //Set address
    struct sockaddr_in address; 
    memset( &address, 0, sizeof(address) );
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((u_short) PORT);

    //Define master socket
    int masterSocket;
    if ((masterSocket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
        perror("socket");
        exit( -1 );
    }

    //Add Socket options
    int option = 1;
    setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
		       (char *) &option, sizeof( int ) );
}