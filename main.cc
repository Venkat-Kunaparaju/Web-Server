#include <stdio.h>
#include<stdlib.h>


#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>




//Constants
#define PORT 1500
#define MAXQ 5
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
        perror("Master socket error");
        exit( -1 );
    }

    //Add Socket options
    int option = 1;
    if ((setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, 
		    (char *) &option, sizeof( int ) )) <= 0) {
        perror("Socket options error");
        exit(-1);
    }

    //Bind address and port to master socket
    if (bind( masterSocket, (struct sockaddr *)&address, 
            sizeof(address) ) <= 0) {
        perror("Binding error");
        exit(-1);
    }

    //Set socket in listen mode to read user requests
    if (listen(masterSocket, MAXQ) <= 0) {
        perror("Listening error");
        exit(-1);
    }

    



    

}