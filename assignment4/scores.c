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

/* int connect_to_server(struct in_addr* ipAddress, int port):
**      - Attempts to connect to server on specifified port
**      - Returns file descriptor
*/
int connect_to_server(struct in_addr* ipAddress, int port) {
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
    **/
    socketAddr.sin_family = AF_INET;    /* IP v4 */
    socketAddr.sin_port = htons(port);  /* port number in network byte order */
    socketAddr.sin_addr.s_addr = ipAddress->s_addr; /* IP address */

    /* Attempt to connect to remote address */
    if(connect(fd, (struct sockaddr*) &socketAddr, sizeof(socketAddr)) < 0) {
        fprintf(stderr, "Bad Server\n");
        exit(5);
    }

    return fd;
}

/* void send_scores_request(int fd)
**  - sends the string "scores\n" to the server
**  - Returns nothing
*/ 
void send_scores_request(int fd) {
    char* requestString;
    requestString = (char*)malloc(strlen("scores\n"));
    sprintf(requestString, "scores\n");

    /* Send request to server
    ** Should really check that all of text was sent - write() returns the 
    ** number of bytes sent.
    */
    if(write(fd, requestString, strlen(requestString)) < 1) {
        fprintf(stderr, "System Error\n");
        exit(8);
    }
}

/* void get_response(int fd):
**  - Gets the reponse back from the server and prints it to STDOUT
**  - Returns nothing
*/ 
void get_response(int fd) {
    char buffer[1024];
    int numBytesRead;
    int eof = 0;

    while(!eof) {
        /* Read up to 1024 bytes from file descriptor. Block until data (or
        ** EOF) is available
        */
        numBytesRead = read(fd, buffer, 1024);
        if(numBytesRead < 0) {
            fprintf(stderr, "System error\n");
            exit(8);
        } else if(numBytesRead == 0) {
            eof = 1;
        } else {
            /* Bytes returned from server - print them out to 
            ** standard output.
            */
            fwrite(buffer, sizeof(char), numBytesRead, stdout);
        }
    }
}

int main(int argc, char* argv[]) {
    int fd;
    int port = 0;
    struct in_addr* ipAddress;
    char* hostname;

    /* If incorrect number or arguments are given */
    if(!((argc == 2) || (argc == 3))) {
        fprintf(stderr, "Usage: scores port [host]\n");
        exit(1);
    }
    /* Parse Port number  */
    port = atoi(argv[1]);
    if(port < 1 || port > 65535) {
        fprintf(stderr, "Invalid Port\n");
        exit(4);
    }

    /* Parse hostname  */
    if(argc == 3) {
        hostname = argv[2];
    } else {
        hostname = "moss";
    }

    /* Convert hostname to IP address */
    ipAddress = name_to_IP_addr(hostname);
    if(!ipAddress) {
        fprintf(stderr, "Bad Server\n");
        exit(5);
    }
       
    /* Connect to IP on given port, send  */
    fd = connect_to_server(ipAddress, port);
    
    /* Send request to recieve scores */
    send_scores_request(fd);

    /* Get the response from the server */
    get_response(fd);
    close(fd);
    return 0;
}

