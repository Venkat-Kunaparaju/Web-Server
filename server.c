#include <stdio.h>
#include <stdlib.h>


#include <sys/socket.h>
#include <unistd.h>
#include <string.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <pthread.h>
#include <time.h>

//Constants
#define PORT 1500
#define MAXQ 100
#define MAXFILELENGTH 700
#define MAXTYPELENGTH 15
#define MAXTOPICLENGTH 50
#define MAXUSERNAMELENGTH 50
#define MAXMESSEAGELENGTH 500
#define MAXOUTPUTLENGTH 1000
#define BOARDSIZE 644
#define MAXMESSAGES 100

char *clrf = "\r\n";
char *messages[MAXMESSAGES];
int numOfMsgs = 0;

pthread_mutex_t mutex;

void processRequest(int);
void threadRequest(int);

int main() {
    if (pthread_mutex_init(&mutex, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return 1;
    }

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

        //Create thread for each client request
        pthread_t t;
        pthread_attr_t attr;
        pthread_attr_init(&attr);
        pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);


        //Process client request
        pthread_create(&t, &attr, (void *(*)(void *)) threadRequest, (void *)slaveSocket);

    }
   

}
void threadRequest(int socket) {
    pthread_mutex_lock(&mutex);
    processRequest(socket);
    shutdown(socket, SHUT_RDWR);
    pthread_mutex_unlock(&mutex);
}

void processRequest(int socket) {

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
        //fprintf(stderr, "%c", hold);
    }
    
    //Open Board
    int fd = -1;
    char *type = (char *)malloc(MAXTYPELENGTH);
    //fprintf(stderr, "%s", file);
    fd = open("board.html", O_RDWR, 0664);
    strcpy(type, "text/html");
    


    //Fields for new message
    char *topic = (char *)malloc(MAXTOPICLENGTH + 1);
    char *username = (char *)malloc(MAXUSERNAMELENGTH + 1);
    char *message = (char *)malloc(MAXMESSEAGELENGTH + 1);
    memset(topic, '\0', MAXTOPICLENGTH + 1);
    memset(username, '\0', MAXUSERNAMELENGTH + 1);
    memset(message, '\0', MAXMESSEAGELENGTH + 1);

    //Check if message request, then update page
    char buff[7];
    memcpy(buff, &file[1], 6);
    buff[6] = '\0';
    if (strcmp(buff, "?topic") == 0) {
        
        //Get time
        time_t tim = time(NULL);
        struct tm *tm = localtime(&tim);
        char datetime[64];
        size_t ret = strftime(datetime, sizeof(datetime), "%c", tm);

        int t = 0;
        int u = 0;
        int m = 0;
        int i = 8;
        //Get topic
        for (i; i < MAXFILELENGTH; i ++) {
            if (file[i] == '&') {
                break;
            }
            topic[t] = file[i];
            t += 1;
        }

        //Get Username
        i += 6;
        for (i; i < MAXFILELENGTH; i ++) {
            if (file[i] == '&') {
                break;
            }
            username[u] = file[i];
            u += 1;
        }

        //Get Message
        i += 5;
        int newlineCounter = 0;
        for (i; i < MAXFILELENGTH; i ++) {
            if (file[i] == '&') {
                break;
            }
            if (file[i] == '+') {
                message[m] = ' ';
            }  else {
                message[m] = file[i];
            }
            m += 1;
            newlineCounter += 1;
            if (newlineCounter >= 40 && message[m-1] == ' ') {
                message[m] = '<';
                message[m + 1] = 'b';
                message[m + 2] = 'r';
                message[m + 3] = '>';
                m += 4;
                newlineCounter = 0;
            }
        }

        //fprintf(stderr, "%s", message);


        //Define Output Message
        char *output = malloc(MAXOUTPUTLENGTH + 1);
        memset(output, '\0', MAXTOPICLENGTH + 1);

        strcat(output, "<html> <body> <p>");
        strcat(output, "<b>");
        strcat(output, topic);
        strcat(output, "</b>");
        strcat(output, " - ");
        strcat(output, datetime);
        strcat(output, "</p><p>");
        strcat(output, message);
        strcat(output, "</p><p> Posted By: ");
        strcat(output, "<b>");
        strcat(output, username);
        strcat(output, "</b>");
        strcat(output, "</p> </body>  </html> ");
        strcat(output, "\n");


        //fprintf(stderr, "%s\n", output);

        //Add to list of messages
        messages[numOfMsgs] = output;
        numOfMsgs += 1;

        //free(output);
    }


    

    //Execute request by creating a child process and wait before exiting to ensure request is met before closing socket
    int ret = fork();
    if (ret == 0){
        //Valid file
        if (fd != -1) {
            
            lseek(fd, 0, SEEK_SET);
            int length = lseek(fd, 0L, SEEK_END);
            lseek(fd, BOARDSIZE, SEEK_SET);

            //fprintf(stderr, "%d", length);
            char lengthStr[100];
            sprintf(lengthStr, "%d", length);

            write(socket, "HTTP/1.1 200 OK", strlen("HTTP/1.1 200 OK"));
            write(socket, clrf, 2);
            write(socket, "Content-length: ", strlen("Content-length: "));
            write(socket, lengthStr, strlen(lengthStr));
            write(socket, clrf, 2);
            write(socket, "Content-type: ", strlen("Content-type: "));
            write(socket, type, strlen(type));
            write(socket, clrf, 2);
            write(socket, clrf, 2);
            //Transfer text from file to client request
            int ret = fork();
            if (ret == 0) {
                for (int i = numOfMsgs-1; i >= 0; i --) {
                    for (int x = 0; x <strlen(messages[i]); x++) {
                        write(fd, &messages[i][x], 1);
                        fprintf(stderr, "%c", messages[i][x]);
                    }
                }
            }
            waitpid(ret, NULL, 0);
            
            lseek(fd, 0L, SEEK_SET);
            while(read(fd, &hold, 1)) {
                write(socket, &hold, 1);
            }
        }
    }
    waitpid(ret, NULL, 0);

    //Close and free unused vars
    close(fd);
    free(type);
    //free(file);

}

