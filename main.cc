#include <stdio.h>
#include<stdlib.h>


#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>

//Constants
#define PORT 1500
#define MAXQ 5
#define MAXFILELENGTH 100

void processRequest(int);

int main() {
    

    //Set address
    struct sockaddr_in address; 
    int aSize = sizeof(address);
    memset( &address, 0, aSize );
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons((u_short) PORT);


    int error;
    //Define master socket
    int masterSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (masterSocket <= 0) {
        perror("Master socket error");
        exit( -1 );
    }
    
    //Add Socket options
    int option = 1;
    error = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &option, sizeof(option));
    if (error) {
        perror("Socket options error");
        exit(-1);
    }

    //Bind socket to address
    error = bind( masterSocket, (struct sockaddr *)&address, aSize );
    if ( error ) {
        perror("Binding error");
        exit( -1 );
    }
    
    
    //Set socket in listen mode to read user requests
    error = listen(masterSocket, MAXQ);
    if (error) {
        perror("Listening error");
        exit(-1);
    }

    //Continuously serve clients
    while(1) {
        // Accept incoming client connections
        int slaveSocket = accept( masterSocket,
                    (struct sockaddr *)&address,
                    (socklen_t*)&aSize);

        if ( slaveSocket < 0 ) {
            perror( "Accept error" );
            exit( -1 );
        }

        //Process client request
        processRequest(slaveSocket);
        

        //Close client connection
        close(slaveSocket);
    }
   

}

void processRequest(int socket) {
    const char * prompt = "Hello";
    write(socket, prompt, strlen(prompt));
    

    //Read http request
    unsigned char hold;

    //Get file name
    char *file = (char *)malloc(MAXFILELENGTH + 1);
    memset(file, '\0', MAXFILELENGTH+1);
    int i = 0;
    while(read(socket, &hold, 1)) {
        if (hold == ' ') {
            while(read(socket, &hold, 1) && hold != ' ') {
                file[i] = hold;
                i += 1;
            }
            break;
        }
    }
    
    int fd;
    //Set file to home directory if first request
    if (strcmp("/", file) == 0 ) {
        fd = open(file, O_RDONLY, 0664);
    }


    free(file);




}