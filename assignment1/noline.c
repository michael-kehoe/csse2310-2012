/* Note: I have read Joel's flip.c code from 2011 as an aid to this assignment.
** After discussing this with Joel on Friday March 16, he recommended that I 
** put this discalimer on my code. I have fully written this assignment by 
** myself, without looking at anyone else's code.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Define Game characters */
#define BLANK '.'
#define P1_CHAR 'O'
#define P2_CHAR 'X'

/* Define game errors */
#define BAD_ARGUMENTS 1
#define BAD_DIMENSIONS 2
#define BAD_PLAYER 3
#define BAD_FILE 4

/* Define game messages/states */
#define BOARD_FULL 0
#define GAME_OVER 1
#define END_OF_FILE 2
#define PLAYING 3


/* The Game Board struct:
** Holds the game information in a struct that can be pointed to with different
** functions.
*/
typedef struct {
    int gameDimensions; /* Dimension of board  */
    char playerTypes[2];    /* Player Types (Human, Computer)  */
    int playerTurn; /* Which player's turn is it  */
    int playerTurnChar[2];  /* Keeps an array of the player characters  */
    int gameStatus; /* Keeps the status of the game  */
    int emptyCells; /* How many empty cells are thre on the board  */
    char** gameData;    /* Stores the board rows  */        
    FILE* in[2];    /* Keeps the input source for O and X */
    FILE* out[2];   /* Keeps the output source for O and X */
} GameBoard;


/* Define protoype functions */
void print_game_error(int error) ;
void print_game_output(GameBoard* board);
GameBoard* create_board(int gameDimensions, char playerXType, 
        char playerOType);
void print_board(GameBoard* board);
char input_in_bounds(GameBoard* board, int xDimension, int yDimension);
char* get_input(GameBoard* board);
int good_move(GameBoard* board, int xDimension, int yDimension);
int check_line(GameBoard* board, int xDimension, int yDimension, int i, int j);
void create_new_game(int argc, char** argv);
void next_player_turn(GameBoard* board);
void play_game(GameBoard* board);
void move(GameBoard* board);
void ai_player_move(GameBoard* board, int playerType);
void check_for_win(GameBoard* board, int xDimension, int yDimension);


/* void print_game_error(int error):
**	- Returns nothing
**	- Prints game error messages out to stderr(Standard error) given error code
**	"error"
*/
void print_game_error(int error) {
    switch(error) {
        case BAD_DIMENSIONS : 
            fprintf(stderr, "Invalid board dimension.\n"); 
            break;
        case BAD_ARGUMENTS : 
            fprintf(stderr, "Usage: noline dim [playerXtype [playerOtype [Oin Oout Xin Xout]]]\n"); 
            break;
        case BAD_PLAYER : 
            fprintf(stderr, "Invalid player type.\n"); 
            break;
        case BAD_FILE : 
            fprintf(stderr, "Invalid files.\n"); 
            break;
    }
}

/*	void print_game_messages(GameBoard* board, int code):
**	- Returns nothing
**	- Prints game status messages to the output specified in the game arguments 
**	- Uses "code" to determine which message to print.
**	- Uses board->out to determine which output to print to.
*/
void print_game_messages(GameBoard* board, int code) {
    switch(code) {
        /* Print message for when the board is full */
        case BOARD_FULL :
            if ((board->out[0] == stdout) && (board->out[1] == stdout)) { 
                fprintf(stdout, "The game is a draw.\n");
            } else if ((board->out[0] == stdout) && (board->out[1] != stdout)) 
					{ 
                fprintf(stdout, "The game is a draw.\n");
                fprintf(board->out[1], "The game is a draw.\n");
            } else if ((board->out[0] != stdout) && (board->out[1] == stdout)) 
					{  
                fprintf(board->out[0], "The game is a draw.\n");
                fprintf(stdout, "The game is a draw.\n");
            } else {
                fprintf(board->out[0], "The game is a draw.\n");
                fprintf(board->out[1], "The game is a draw.\n");
            }
            break;
			
        /* Print message for when the game is over */
        case GAME_OVER : 
            if((board->out[0] == stdout) && (board->out[1] == stdout)) {
                 fprintf(stdout, "Player %c loses.\n", board->playerTurnChar
						[!board->playerTurn]);
            } else if((board->out[0] == stdout) && (board->out[1] != stdout)) {
                 fprintf(stdout, "Player %c loses.\n", board->playerTurnChar
						[!board->playerTurn]);
                 fprintf(board->out[1], "Player %c loses.\n", 
						board->playerTurnChar[!board->playerTurn]);
            } else if((board->out[0] != stdout) && (board->out[1] == stdout)) {
                 fprintf(board->out[0], "Player %c loses.\n", 
						board->playerTurnChar[!board->playerTurn]);
                 fprintf(stdout, "Player %c loses.\n", 
						board->playerTurnChar[!board->playerTurn]);
            } else {
                fprintf(board->out[0], "Player %c loses.\n", 
						board->playerTurnChar[!board->playerTurn]);
                fprintf(board->out[1], "Player %c loses.\n", 
						board->playerTurnChar[!board->playerTurn]);
            }
            break;
		
        /* Print message for when a player reaches EOF */
        case END_OF_FILE :
            if ((board->out[0] == stdout) && (board->out[1] == stdout)) {
                fprintf(stdout, "Player %c loses due to EOF.\n", 
						board->playerTurnChar[board->playerTurn]);
            } else if ((board->out[0] == stdout) && (board->out[1] != stdout)) 
					{
                fprintf(stdout, "Player %c loses due to EOF.\n", 
						board->playerTurnChar[board->playerTurn]);
                fprintf(board->out[1], "Player %c loses due to EOF.\n", 
						board->playerTurnChar[board->playerTurn]);
            } else if ((board->out[0] != stdout) && (board->out[1] == stdout)) 
					{
                fprintf(board->out[0], "Player %c loses due to EOF.\n", 
						board->playerTurnChar[board->playerTurn]);
                fprintf(stdout, "Player %c loses.\n", board->
						playerTurnChar[board->playerTurn]);
            }  else {
                fprintf(board->out[0], "Player %c loses due to EOF.\n", 
						board->playerTurnChar[board->playerTurn]);
                fprintf(board->out[1], "Player %c loses due to EOF.\n", 
						board->playerTurnChar[board->playerTurn]);
            }            
            break;
    }
}
/*GameBoard* create_board(int gameDimensions,char playerXType,char playerOType)
**	- Returns GameBoard struct
**	- Creates the gameboard struct and initalises the data within the struct.
**	- It sets the game dimensions, player types and sets defaults for the input
**	and output streams
*/ 
GameBoard* create_board(int gameDimensions, char playerXType, 
		char playerOType) {
    int i, j = 0;

    /* Malloc the game board */
    GameBoard* board = (GameBoard*)malloc(sizeof(GameBoard));
    board->gameData = (char**)malloc(sizeof(char*) * gameDimensions);
	
    /* Fill the board with empty characters  */
    for(i = 0; i < gameDimensions; ++i) {
        board->gameData[i] = (char*)malloc(sizeof(char)*(gameDimensions+1));
        for(j = 0; j < gameDimensions; ++j) {
            board->gameData[i][j] = BLANK;
        }
        board->gameData[i][j] = '\0';
    }
	
    /* Set the other game parameters  */
    board->gameDimensions = gameDimensions;
    board->gameStatus = PLAYING;
    board->playerTypes[0] = playerXType;
    board->playerTypes[1] = playerOType;
    board->playerTurn = 0;
    board->playerTurnChar[0] = P1_CHAR;
    board->playerTurnChar[1] = P2_CHAR;
    board->emptyCells = gameDimensions*gameDimensions + 1; /* Add 1 so the 
			board full works properly */
	
    /* Defautly setup to use stdin and stdout for printing game information */
    board->in[0] = stdin;
    board->in[1] = stdin;
    board->out[0] = stdout;
    board->out[1] = stdout;
    return board;
}

/* 	void print_board(GameBoard* board):
**	Print out the gameboard to the correct stream. The board input retrieves
**	the gameboard data.
*/
void print_board(GameBoard* board) {
        int i;
        for (i = 0; i < board->gameDimensions; ++i) {
            fprintf(board->out[!board->playerTurn], "-");
        }
        fprintf(board->out[!board->playerTurn], "\n");
        for(i = 0; i < board->gameDimensions; ++i) {
            fprintf(board->out[!board->playerTurn], "%s\n", 
					board->gameData[i]);
        }
        for (i = 0; i < board->gameDimensions; ++i) {
            fprintf(board->out[!board->playerTurn], "=");
        }
        fprintf(board->out[!board->playerTurn], "\n");
}

/*	next_player_turn(GameBoard* board):
**	Set the 'other' player to have the next turn
*/
void next_player_turn(GameBoard* board) {
    board->playerTurn = (board->playerTurn + 1) % 2;
}

/*  input_in_bounds(GameBoard* board, int xDimension, int yDimension):
**	- returns char(which is used as a bool). 
**	Check if the xDimension and yDimension provided in the arguments are within
**	the bounds of the board
*/
char input_in_bounds(GameBoard* board, int xDimension, int yDimension) {
    return ((xDimension >= 0) && (xDimension < board->gameDimensions) && 
			(yDimension >= 0) && (yDimension < board->gameDimensions));
} 

/*	char* get_input(GameBoard* board):
**	- returns char array buffer.
**	Get the input from the current player filestream(using board struct)
**	and checks for empty cells and end of file. If these conditions exist, it 
**	determines who wins the game.
*/
char* get_input(GameBoard* board) {
    char* buffer = (char*)malloc(sizeof(char) * 80);
	
	/* Check if all the cells have been used. */
    if(board->emptyCells == 1) {
        board->gameStatus = BOARD_FULL;
        print_game_messages(board, BOARD_FULL);
        exit(0);
    }
	/* Puts 80 characters into the buffer. */
    fgets(buffer, 80, board->in[board->playerTurn]);
	
	/* Check for end of file */
    if((feof(board->in[board->playerTurn]) != 0)) {
        board->gameStatus = END_OF_FILE;
        next_player_turn(board);
        print_board(board);
        next_player_turn(board);
        print_game_messages(board, END_OF_FILE);
        exit(0);
    }
    return buffer;
}

/*	int good_move(GameBoard* board, int xDimension, int yDimension):
**	- returns integer. 1 for successful move, 0  for unsuccessful move.
**	checks if there is anything in the xDimension, yDimension co-ordinate and 
**	places the players character into that square. Also decrements the 
**	emptyCells counter.
*/
int good_move(GameBoard* board, int xDimension, int yDimension) {
    if(board->gameData[xDimension][yDimension] == BLANK) {
        board->gameData[xDimension][yDimension] = board->playerTurnChar[
				board->playerTurn];
        board->emptyCells--;
        return 1;
    }
    else return 0;
}

/*	check_line(GameBoard* board, int xDimension, int yDimension, int i, int j):
**	- returns integer. If a line exists, return 1, else return 0.
**	Checks if line exists in the board. Checks from  int xDimension, 
**	int yDimension and looks in the direction of i,j and if the same character 
**	is found, look at -i,-j.
*/
int check_line(GameBoard* board, int xDimension, int yDimension, int i, int j) 
		{
    int newX, newY = 0;
    newX = xDimension - i;
    newY = yDimension - j;
	
	/* Checks if the cell we are looking at is in the bounds of the board
		and then continues algorithmn */	
    if(input_in_bounds(board, newX, newY)) {
		
		/* If correct character is found in cell we are looking in */
        if(board->gameData[newX][newY] == board->playerTurnChar[
				board->playerTurn]) {
			/* Check next cell. */
            if(input_in_bounds(board, newX - i, newY - j)) {
                if(board->gameData[newX-i][newY-j] == board->playerTurnChar[
						board->playerTurn]) {
                    return 1;
                }
            }
			/* Check corresponding cell at -i,-j*/
            else if(input_in_bounds(board, newX+i+i, newY+j+j)) {
                if(board->gameData[newX+i+i][newY+j+j] == 
					board->playerTurnChar[board->playerTurn]) {
                   return 1;
                }
            }
        }
    }
    return 0;
}

/*	check_for_win(GameBoard* board, int xDimension, int yDimension):
**	-returns nothing
**	Check if all the game cells have been taken.
**	Else check in cells around where the character was just placed.
**	Call on check_line to verify if the game has been won.
*/
void check_for_win(GameBoard* board, int xDimension, int yDimension) {
    int i, j = 0;
	
	/*  Check if all the cells have been taken. */
    if(board->emptyCells == 0) {
        /* Set the game status to full board(drawn game)  */
        board->gameStatus = BOARD_FULL;
    }
    /* Else all surrounding squares */
    else {
        for(i = -1; i < 2; ++i) {
            for(j = -1; j < 2; ++j) {
                if((i != 0) || (j != 0)) {
                    /* If there is a win, set the game status to be over  */
                    if(check_line(board, xDimension, yDimension, i, j)) {
                        board->gameStatus = GAME_OVER;
                        break;
                    }    
                }
            }
        }
    }
}

/* 	void ai_player_move(GameBoard* board, int playerType):
**	-returns nothing.
**	Creates the AI player move. Determines which computer algorithm to use via 
**	the playerType variable. The functions will also check for a win.
*/
void ai_player_move(GameBoard* board, int playerType) {
    int n, s, k;
    int col, row;
    int valid;
    valid = 0;
    n = 0;
    k = 0;
    s = board->gameDimensions;
    switch(playerType) {
        case 1:
            do {     
                k = n * ( s + 2 ) % ( s * s);
                row = k / s;
                col = k % s;
                if(input_in_bounds(board, row, col)) {
                    if(good_move(board, row, col)) {
                        fprintf(board->out[board->playerTurn], "%c> %d %d\n", 
								board->playerTurnChar[board->playerTurn], row, 
										col);
                        check_for_win(board, row, col);
                        valid = 1;
                        break;
                    } else n++;
                } 
            } while(valid == 0);
            break;    
        case 2: 
            do {
                k = n * ( s + 2) % (s * s);
                row = ( s - 1) - (k / s);
                col = (s - 1) - (k % s);
                if(input_in_bounds(board, row, col)) {
                    if(good_move(board, row, col)) {
                        fprintf(board->out[board->playerTurn], "%c> %d %d\n", 
								board->playerTurnChar[board->playerTurn], row, 
										col);
                        check_for_win(board, row, col);
                        valid = 1;
                        break;
                    } else n++;
                }               
            } while(valid == 0);
            break;
    }
}

/*	void move(GameBoard* board):
**	-returns nothing.
**	Using the switch statement to select what player type to set the move on.
**	Handles input logic for human players and then checks the validity of the
**	input. 
**	If the player types are computer, it points to the ai_player_move function.
*/
void move(GameBoard* board) {
    char* move_entered;
    int valid = 0;
    int xDimension, yDimension;
	
	/* Select which player to move */
    switch(board->playerTypes[board->playerTurn]) {
        case 0: do {
					/*  Displays prompt */
                   if(board->emptyCells != 1) {
                        fprintf(board->out[board->playerTurn], "%c> ", 
								board->playerTurnChar[board->playerTurn]);
                    } 
					/*  Get input */
                    move_entered = get_input(board);
                    if(sscanf(move_entered, "%d %d", &xDimension, &yDimension) 
							== 2) {
						/* Check for win */
                        if(input_in_bounds(board, xDimension, yDimension)) {
                            if(good_move(board, xDimension ,yDimension)) {
                                check_for_win(board, xDimension, yDimension);
                                valid = 1;     
                             }
                         }
                    }
                    free(move_entered);             
                } while(valid == 0);
                break;
        case 1:
            ai_player_move(board, 1);
            break;
        case 2:
            ai_player_move(board, 2);
            break; 
    }
}

/*	void play_game(GameBoard* board):
**	-returns nothing
**	Starts the game playing logic. The function keeps cycling between players.
**	Sets end of game messages.
*/
void play_game(GameBoard* board) {
	/*  Keep looping until game is not playing. */
    do {
    move(board);
    board->playerTurn = ( board->playerTurn + 1 ) % 2;
    print_board(board);
    } while(board->gameStatus == PLAYING);
    print_game_messages(board, board->gameStatus);      
}
/* 	void process_stream_arguments(GameBoard* board, int argc, char** argv):
**	-reutrns nothing
**	Processes last 4 arguments program
*/
void process_stream_arguments(GameBoard* board, int argc, char** argv) {  
	/* If there more than 4 arguments */
    if(argc > 4) {
        /* Check for the Oin input  */
        if(strcmp(argv[4], "-") == 0) {
            board->in[0] = stdin;
        } else if(fopen(argv[4], "r") != NULL) {
            /*Use filestream*/
            board->in[0] = fopen(argv[4], "r");
        }else {
            print_game_error(BAD_FILE);
            exit(BAD_FILE);
        } 
        
        /* Check the Oout input */
        if(strcmp(argv[5], "-") == 0) {
            board->out[0] = stdout;
        } else if(fopen(argv[5], "w") != NULL){
            /*Use filestream*/
            board->out[0] = fopen(argv[5], "w");
        }
        else {
            print_game_error(BAD_FILE);
            exit(BAD_FILE);
        }

        /* Check for the Xin input */
        if(strcmp(argv[6], "-") == 0) {
            board->in[1] = stdin;
        } else if(fopen(argv[6], "r") != NULL) {
            /*Use filestream*/
            board->in[1] = fopen(argv[6], "r");
        }
        else {
            print_game_error(BAD_FILE);
            exit(BAD_FILE);
        }

        /* Check for the xOut input */
        if(strcmp(argv[7], "-") == 0) {
            board->out[1] = stdout;
        } else if(fopen(argv[7], "w") != NULL) {
            /*Use filestream*/
            board->out[1] = fopen(argv[7], "w");
        } else {
            print_game_error(BAD_FILE);
            exit(BAD_FILE);
        }
    }
}                

/*	void create_new_game(int argc, char** argv):
**	-returns nothing
**	Parses first 3 arguments. Creates board. Prints intial board and starts 
**	playing logic.
*/
void create_new_game(int argc, char** argv) {
    int gameDimensions = 0;
    char* error;
    short playerXType = 0;
    short playerOType = 0;
    GameBoard* board = 0;
    gameDimensions = (int)strtol(argv[1], &error, 10);
    if ((*error != '\0') || (gameDimensions < 2) || !(gameDimensions % 2)) {
        print_game_error(BAD_DIMENSIONS);
        exit(BAD_DIMENSIONS);
    }
    if (argc > 2) {
        playerXType = (int)strtol(argv[2], &error, 10);
        if ((*error != '\0') || (playerXType < 0) || (playerXType > 2)) {
            print_game_error(BAD_PLAYER);
            exit(BAD_PLAYER);
        }
        if (argc > 3) {
            playerOType = (int)strtol(argv[3], &error, 10);
            if ((*error != '\0') || (playerOType < 0) || (playerOType > 2)) {
                print_game_error(BAD_PLAYER);
                exit(BAD_PLAYER);
            }
        }   else {
                playerOType = 0;
            }
    }
    else {
        playerXType = 0;  
    }
    /* Print the board to player O */
    board = create_board(gameDimensions, playerOType, playerXType);
    process_stream_arguments(board, argc, argv); 
    print_board(board);
    play_game(board);
} 

int main(int argc, char** argv) { 
    if ((argc == 2) || (argc == 3) || (argc == 4) || (argc == 8)) {
        create_new_game(argc, argv);
    }  
    else {
        print_game_error(BAD_ARGUMENTS);
        exit(BAD_ARGUMENTS);
    }  
    return 0; 
}

