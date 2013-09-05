/* Disclaimer: using code from Peter Sutton's lectures.
** All code that isn't my own is written by Dr Sutton and is posted
** on the course website.
*/
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h> 
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h>

int fd;
int numResponses = -1;
int gameOver = 0;
FILE* filetest, *sendtest;
pthread_mutex_t mutex;

/* struct in_addr* name_to_IP_addr(char* hostname):
**      - returns IP struct
**      - Converts hostname into IP 
*/
struct in_addr* name_to_IP_addr(char* hostname) {
    int error;
    struct addrinfo* addressInfo;

    /* Converting hostname into address information (IP address) */
    error = getaddrinfo(hostname, NULL, NULL, &addressInfo);
    if(error) {
        return NULL;
    }
    /* Extract IP address and return */
    return &(((struct sockaddr_in*)(addressInfo->ai_addr))->sin_addr);
}

/* int connect(struct in_addr* ipAddress, int port):
**      - Attempts to connect to server on specifified port
**      - Returns file descriptor as an integer
*/
int connect_to(struct in_addr* ipAddress, int port) {
    struct sockaddr_in socketAddr;
    int fd;
    
    /* Create TCP socket */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        fprintf(stderr, "Bad Server\n");
        exit(5);
    }

    /* Set up an address structure that contains the address
    ** (IP address and port number) that we're trying to connect to.
    */
    socketAddr.sin_family = AF_INET;    /* IP v4 */
    socketAddr.sin_port = htons(port);  /* port number in network byte order */
    socketAddr.sin_addr.s_addr = ipAddress->s_addr; /* IP address */

    /* Attempt to connect to remote address */
    if(connect(fd, (struct sockaddr*)&socketAddr, sizeof(socketAddr)) < 0) {
        fprintf(stderr, "Bad Server\n");
        exit(5);
    }
    filetest = fdopen(fd, "r");
    sendtest = fdopen(fd, "w");
    return fd;
}

/* void send_name(char* names):
**      - Sends client name to server
**      - Returns nothing. 
*/
void send_name(char* names) {
    char* requestString;

    requestString = (char*)malloc(strlen(names+1));

    if(fprintf(sendtest, "%s\n", names) == -1) {
        fprintf(stderr, "System Error\n");
        exit(8);
    } else {
        fflush(sendtest);
    }
}

/* void *send_response(void *vargp):
**      - Send response to questions
**      - Checks for a valid input
**      - Returns nothing.
*/ 
void *send_response(void *vargp) {
    
    char* buffer = (char*)malloc(sizeof(char) * 1024);
    int response; 
    while(1) {
        /* Check for EOF on stdin */
        fgets(buffer, 1024, stdin);
        if(sscanf(buffer, "%d\n", &response) == 1) {
            if((response <= numResponses) && (response > 0)) {
                if(fprintf(sendtest, "%s\n", buffer) == -1) {
                    /* Check for disconnect on fflush() */
                    fprintf(stderr, "System Error\n");
                    exit(8);
                } else {
                    fflush(sendtest);
                }
            } else {
                fprintf(stdout, "Invalid guess\n");
            }
        } else {
            fprintf(stdout, "Invalid guess\n"); 
        }          
    }    
}


/* void *get_response(void *vargp):
**      - Gets the response from the server
**      - Processes the response and outputs appropriate message to cloemt
**      - Checks for errors with the server protocol
**      - Returns nothing.
*/ 
void *get_response(void *vargp) {
    char buffer[1024];
    int stateMachine = 0, questNum = 0, response = 0, winnerFound;
    char* winnerbuffer = malloc(sizeof(char)*2048);
     
    while(fgets(buffer, 1024, filetest) != NULL) {
        if(strncmp(buffer, "$\n", 1) == 0) {
            fprintf(stderr, "Server Full\n");
            exit(11);
        }
        /* Hello Player */
        if((strncmp(buffer, "Hello Player ", 13) == 0) && (stateMachine ==0)) {
            fprintf(stdout, "%s", buffer);
            stateMachine = 1;
        } else if((strncmp(buffer, "S", 1) == 0) && (stateMachine == 1)) {
            /* If scores are sent  */
            memmove(buffer, buffer+1, strlen(buffer));
            questNum = 0;
            stateMachine = 3;
            if(winnerFound == 1) {
                fprintf(stdout, "Scores: %s", buffer);
                fprintf(stdout, "Winner(s): %s", winnerbuffer);
                exit(0);
            } else {
                fprintf(stdout, "Scores: %s\n", buffer);
            }
        } else if((stateMachine == 3) && (!strncmp(buffer, ".", 1) == 0)) { 
            /* Print out the question  */
            fprintf(stdout, "%s", buffer);
            stateMachine = 3;
        } else if((strncmp(buffer, ".", 1) == 0) && (stateMachine == 3)) {
            /* Print the =====  */
            fprintf(stdout, "=====\n");
            stateMachine = 4;
        } else if((sscanf(buffer, "%d\n", &response) == 1) && (stateMachine == 4)) {
            /* Check for the number of answers  */
            numResponses = response;
            stateMachine = 5;
        } else if((stateMachine == 5) && (questNum < numResponses)) {
            fprintf(stdout, "%d: %s", questNum+1, buffer);
            questNum++;
        } else if((strncmp(buffer, "C", 1) == 0) && (stateMachine == 6)) {
            memmove(buffer, buffer+1, strlen(buffer));
            fprintf(stdout, "Results: %s", buffer);
            stateMachine = 1;
        } else if(strncmp(buffer, "W", 1) == 0) {
            memmove(buffer, buffer+1, strlen(buffer));
            memset(winnerbuffer, 0, 2048); 
            strcpy(winnerbuffer, buffer);
            winnerFound = 1;
        } else {
            fprintf(stderr, "Protocol Error\n");
            exit(12);
        }
        if(questNum == numResponses) {
            fprintf(stdout, "++++\n");
            stateMachine = 6;
            questNum = 0;
        } 
    }
    /* if EOF then server disconnected */
    return NULL;
}

int main(int argc, char* argv[]) {
    int port = 0;
    struct in_addr* ipAddress;
    char *hostname, *name;
    pthread_t tid[2];

    /* Disable sigpipe using pthread_sigmask */

    if(!((argc == 3) || (argc == 4))) {
        fprintf(stderr, "Usage: trivial name port [host]\n");
        exit(1);
    }
        
    name = argv[1];
    port = atoi(argv[2]);

    if(port < 1 || port > 65535) {
        fprintf(stderr, "Invalid Port\n");
        exit(4);
    }

    if(argc == 4) {
        hostname = argv[3];
    } else {
        hostname = "moss";
    }

    /* Convert hostname to IP address */
    ipAddress = name_to_IP_addr(hostname);
    if(!ipAddress) {
        fprintf(stderr, "Bad Server\n");
        exit(5);
    }

    fd = connect_to(ipAddress, port);

    send_name(name);    
    
    /* Create two threads */
    pthread_create(&tid[0], NULL, send_response, NULL);
    pthread_create(&tid[1], NULL, get_response, NULL);

    while(1) {} 
        
    close(fd);
    
    return 0;
}
