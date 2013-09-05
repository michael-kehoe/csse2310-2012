/* Using code from Peter Suttons lectures. All code used in this program
** is fully understood by myself as the ultimate end user
*/
 
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>

/* The TriviaGame struct:
** Holds all of the information about the game and is primarily used by the
** the game logic thread.
*/ 
typedef struct {
    int gamePort; /* Port the game is running on */
    FILE* questionFile; /* A file pointer to the question file */
    char* questionFileName; /* The name of the question file */
    int minPlayers; /* The minimum number of players required to play */
    int maxPlayers; /* The maximum number of players required to play */
    int numPlayers; /* The current number of players playing the game */
    int questionTimeout; /* The amount of time clients are given to answer */
    int gameNum; /* The number of the game */
} TriviaGame;

/* The TriviaClient struct:
** Holds information about the client that is connected to the server.
*/ 
typedef struct {
    int score; /* The score the client currently has */
    int qResponse; /* The clients response to the last question */
    int game; /* The number of the game they are playing */
    FILE* readStream; /* Filestream for the game logic to read from */
    FILE* writeStream; /* Filestream for the game logic to write to */
} TriviaClient;

TriviaClient *clients; /* A global array of clients */
TriviaGame *games; /* A global array of games */
int clientCount; /* Total number of clients connected to server */

/* int open_listen(int port): 
**  - creates a file descriptor that listens on 'port'
**  - checks if the port can be bound to
**  - returns an integer which is the filedescriptor
*/ 
int open_listen(int port) {
    int fd;
    struct sockaddr_in serverAddr;
    int optVal;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        fprintf(stderr, "Bad Listen\n");
        exit(6);
    }

    optVal = 1;
    if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int)) < 0) {
        fprintf(stderr, "Bad Listen\n");
        exit(6);
    }
    
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(fd, (struct sockaddr*)&serverAddr, 
            sizeof(struct sockaddr_in)) < 0) {
        fprintf(stderr, "Bad Listen\n");
        exit(6);
    }
    if(listen(fd, SOMAXCONN) < 0) {
        fprintf(stderr, "Bad Listen\n");
        exit(6);
    }

    return fd;
}

/* void* client_thread(void* arg):
**  - Initiates contact with client
**  - Recieves all communication from client and stores it in a client struct
**  - Returns nothing
*/
void* client_thread(void* arg) {
    TriviaClient client;
    int response; 
    client = (TriviaClient)arg;
    char buffer[1024];
    
    /* Checks if server is full */
    if(games[client.game].numPlayers > games[client.game].maxPlayers) {
        fprintf(client.writeStream, "$\n");
    } else {
        /* Welcome client to game */
        fprintf(client.writeStream, "Hello Player %d\\%d\n", 
                games[client.game].numPlayers, games[client.game].minPlayers);
    }

    /* Get any communication from client and store it in client struct */
    while(1) {
        fgets(buffer, 1024, client.readStream);
        sscanf(buffer, "%d\n", &response);
        client.qResponse = response;
    }
}

/* void game_logic_thread(void* arg):
**  - This function runs the game logic
**  - It opens the question file and reads from it and performs
**      most of the server -> client communication.
**  - Returns nothing
*/ 
void game_logic_thread(void* arg) {
    int gameRunning = 1;
    TriviaGame game;
    game = (TriviaGame)arg;

    /* Open the Question file */
    if(fopen(game.questionFileName, "r") != NULL) {
        /* Use filestream */
        game.questionFile = fopen(game.questionFileName, "r");
    } else {
        fprintf(stderr, "Bad File\n");
        exit(3);
    } 

    /* Wait for appropriate number of client to connect */
    while(gameRunning) {    
        while(game.numPlayers < game.minPlayers) {
            
        }       
        /* Reset all players responses */
        for(int i = 0; i < game.numPlayers; i++) {
            clients[i].qResponse = 0;
        }    

        /* Send question */
        for(int i = 0; i < game.numPlayers; i++) {
            
        }
        
        /* Send answers */
        for(int i = 0; i < game.numPlayers; i++) {
        }        
        
        /* Let players respond */
        sleep(game.questionTimeout);

        /* Check question responses */
        for(int i = 0; i < game.numPlayers; i++) {
        }
    }
    
    /* Print winner out to users */
    for(int i = 0; i < game.numPlayers; i++) {
        fprintf(clients[i].writeStream, "Winner(s):\n");
    }
}

/* void process_connections(int fdServer):
**  - waits for connections to specified file descriptor 'fdServer'
**  - When connection is made, creates a new client struct and starts
**      a new client thread
**  - Returns nothing
*/ 
void process_connections(int fdServer) {
    int fd;
    struct sockaddr_in fromAddr;
    socklen_t fromAddrSize;
    pthread_t threadId;

    while(1) {
        fromAddrSize = sizeof(struct sockaddr_in);
        /* Block, waiting for a connection request then accept it */
        /* fromAddr will get populated with client address */
        fd = accept(fdServer, (struct sockaddr*)&fromAddr,  &fromAddrSize);
        if(fd < 0) {
            perror("Bad Client\n");
            exit(7);
        }
        
        /* Create new Client struct */
        TriviaClient client;
        client.readStream = fdopen(fd, "r");
        client.writeStream = fdopen(fd, "w");
        client.game = fd;
     
        /* Start a thread to communicate with client */
        /* Pass client struct to client as last argument */
        pthread_create(&threadId, NULL, client_thread, (void*)client);
        pthread_detach(threadId);
    }
}

int main(int argc, char* argv[]) {
    int port, minplayers, maxplayers, roundtime;
    int fdServer1, gameId = 0, i = 4;;
    
    if(!((argc == 6) || (argc % 2 == 0))) {
        fprintf(stderr, 
                "Usage: serv round_time minplayers maxplayers port qfile [port qfile ...]\n");
        exit(1);
    }
    roundtime = atoi(argv[1]);
    if(roundtime < 1) {
        fprintf(stderr, "Bad Number\n");
        exit(2);
    }
    minplayers = atoi(argv[2]);
    if(minplayers < 1) {
        fprintf(stderr, "Bad Number\n");
        exit(2);
    }
    maxplayers = atoi(argv[3]);
    if ((maxplayers < 1) || (maxplayers < minplayers)) {
        fprintf(stderr, "Bad Number\n");
        exit(2);
    }
    while(i < argc) {
        port = atoi(argv[i]);
        if(port < 1024 || port > 65535) {
            fprintf(stderr,  "Invalid Port\n");
            exit(4);
        }
        
        /* Create new game struct */
        TriviaGame game;
        game.maxPlayers = maxplayers;
        game.minPlayers = minplayers;
        game.questionTimeout = roundtime;
        game.gamePort = port;
        game.questionFileName = argv[i+1];
        
        /* Start Listening on port and create game thread*/
        fdServer1 = open_listen(port);
        game.gameNum = fdServer1;
        process_connections(fdServer1);
        pthread_create(&threadId, NULL, game_logic_thread, (void*)game);   
        i = i+2;
    }
    while(1) {
    }
    return 0;
}
