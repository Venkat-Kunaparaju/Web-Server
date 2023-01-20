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
#define BOARDSIZE 649
#define MAXMESSAGES 100

char *clrf = "\r\n";
int id = 0;

//Arrays to store message interaction data
int numLikes[MAXMESSAGES];
int numDislikes[MAXMESSAGES];
char numUsername[MAXMESSAGES][MAXUSERNAMELENGTH + 1];

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

        pthread_join(t, NULL);

    }
    pthread_mutex_destroy(&mutex);
   

}
void threadRequest(int socket) {
    processRequest(socket);
    shutdown(socket, SHUT_RDWR);
}

void processRequest(int socket) {

    //Read http request
    unsigned char hold;

    //Get file name
    char file[MAXFILELENGTH + 1];
    int i = 0;

    int check = 0;

    int post = 1;
    while(read(socket, &hold, 1)) {

        //Get request
        if (hold == ' ' && check == 3) {
            post = 0;
            while(read(socket, &hold, 1) && hold != ' ') {
                file[i] = hold;
                i += 1;
            }
            break;
            
        }

        //Hold request
        if (hold == ' ' && check == 4) {
            while(read(socket, &hold, 1)) {
                file[i] = hold;
                i += 1;
                if (file[i-1] == 10 && file[i-2] == 13 && file[i-3] == 10 && file[i-4] == 13) {
                    break;
                }
            }
            i = 0;
            while(read(socket, &hold, 1) && hold != '*') {
                file[i] = hold;
                i += 1;
            }
            break;
        }
        check += 1;
        
    }
    file[i] = '\0';

    //fprintf(stderr, "%s", file);


    
    
    //Open Board
    int fd = -1;
    char type[MAXTYPELENGTH];
    //fprintf(stderr, "%s", file);
    fd = open("board.html", O_RDWR | O_APPEND, 0664);
    strcpy(type, "text/html");
    


   
    //if post request then figure out which type of post request


    //Check if message request, then update page
    char buff[8];
    memcpy(buff, &file[0], 5);
    buff[7] = '\0';
    fprintf(stderr, "%s\n", buff);
    if (post == 1) {
        //Send new message
        if (strcmp(buff, "topic") == 0) {
            //Fields for new message
            char topic[MAXTOPICLENGTH + 1];
            char username[MAXUSERNAMELENGTH + 1];
            char message[MAXMESSEAGELENGTH + 1];
            //Get time
            time_t tim = time(NULL);
            struct tm *tm = localtime(&tim);
            char datetime[64];
            size_t ret = strftime(datetime, sizeof(datetime), "%c", tm);

            int t = 0;
            int u = 0;
            int m = 0;
            int i = 6;
            //Get topic
            for (i; i < MAXFILELENGTH; i ++) {
                if (file[i] == '&') {
                    break;
                }
                if (file[i] == '+') {
                    topic[t] = ' ';
                }  else {
                    strncpy(&topic[t], &file[i], 1);
                }
                t += 1;
            }
            topic[t] = '\0';

            

            //Get Username
            i += 6;
            for (i; i < MAXFILELENGTH; i ++) {
                if (file[i] == '&') {
                    break;
                }
                if (file[i] == '+') {
                    username[u] = ' ';
                }  else {
                    strncpy(&username[u], &file[i], 1);
                }
                u += 1;
            }
            username[u] = '\0';
            
            //fprintf(stderr, "username: %s\n", username);
            //fprintf(stderr, "topic: %s\n", topic);
            //Get Message
            i += 5;
            int newlineCounter = 0;
            for (i; i < MAXFILELENGTH; i ++) {
                //fprintf(stderr, "%s", username);
                if (file[i] == '&' || file[i] == '\0') {
                    break;
                }
                if (file[i] == '+') {
                    message[m] = ' ';
                }  else {
                    strncpy(&message[m], &file[i], 1);
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

            message[m] = '\0';

        

            //fprintf(stderr, "message: %s\n", message);
            //fprintf(stderr, "username: %s\n", username);
            //fprintf(stderr, "topic: %s\n", topic);

            //Define Output Message
            char output[MAXOUTPUTLENGTH + 1];

            char tempTopic[MAXTOPICLENGTH+1];
            strcpy(tempTopic, topic);
            
            char tempUsername[MAXUSERNAMELENGTH + 1];
            strcpy(tempUsername, username);

            char tempMessage[MAXMESSEAGELENGTH + 1];
            strcpy(tempMessage, message);

            //free(username);
            //free(message);
            //free(topic);


            pthread_mutex_lock(&mutex);

            strcat(output, "<html> <body> <p>");
            strcat(output, "<b>");
            strcat(output, tempTopic);
            strcat(output, "</b>");
            strcat(output, " - ");
            strcat(output, datetime);
            strcat(output, "</p><p>");
            strcat(output, tempMessage);
            strcat(output, "</p><p> Posted By: ");
            strcat(output, "<b>");
            strcat(output, tempUsername);
            strcat(output, "</b>");
            strcat(output, "</p> ");

            strcat(output, " <div class=\"row-fluid\"> <form action=\"\" method=\"post\"> <button name=\"like");

            char lengthStr[100];
            sprintf(lengthStr, "%d", id);
            numLikes[id] = 0;
            numDislikes[id] = 0;
            memcpy(numUsername[id], username, strlen(username));
            //fprintf(stderr, "%s\n", numUsername[id]);


            id += 1;

            strcat(output, lengthStr);
            strcat(output, "*\" value=\"\"> Like </button> </form> <form action=\"\" method=\"post\"> <button name=\"disl");
            strcat(output, lengthStr);
            strcat(output, "*\" value=\"\"> Dislike </button> </form> ");

            strcat(output, " </body> </html> ");
            

            strcat(output, "\n----------------------------------------\n");

            //Append output to file
            write(fd, output, strlen(output));
            pthread_mutex_unlock(&mutex);

            //free(output);
        }
        //Dislike
        if (buff[0] == 'd') {
            int tempId = buff[4] - '0';
            numDislikes[tempId] += 1;
            //fprintf(stderr, "%s\n", numUsername[tempId]);
            //fprintf(stderr, "Check dislike");

        }
        if (buff[0] == 'l') {
            int tempId = buff[4] - '0';
            numLikes[tempId] += 1;
            //fprintf(stderr, "%d", numLikes[tempId]);
            //fprintf(stderr, "Check like");
        }

    }

    

    //Execute request by creating a child process and wait before exiting to ensure request is met before closing socket
    int ret = fork();
    if (ret == 0){
        //Valid file
        if (fd != -1) {
            
            lseek(fd, 0, SEEK_SET);
            int length = lseek(fd, 0L, SEEK_END);
            lseek(fd, 0, SEEK_SET);

            //fprintf(stderr, "%d", length);
            char lengthStr[100];
            sprintf(lengthStr, "%d", length);
            pthread_mutex_lock(&mutex);

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
            
           
            while(read(fd, &hold, 1)) {
                write(socket, &hold, 1);
            }
            pthread_mutex_unlock(&mutex);
        }
    }
    waitpid(ret, NULL, 0);

    //Close and free unused vars
    close(fd);
    //free(type);
    //free(file);

}

