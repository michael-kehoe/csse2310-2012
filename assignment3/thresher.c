#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

/* Define Exit Status */
#define OK 0
#define BAD_ARGUMENTS 1
#define BAD_PARAM 2
#define EXEC_FAIL 3
#define SYS_CALL_ERROR 4
#define OTHER 5

/* Define Build Types */
#define ANSIC 21
#define C99 22
#define JAVA 23
#define LATEX 24

/* The process object struct:
** Holds the process information in a struct that can be pointed to with 
** different functions.
*/
typedef struct {
    int processId;
    FILE *dataIn;
    FILE *dataOut;
    int running;
    int pipeStatus;
    char *filename;
    int fileType;
    char *compiler;
    char *styleType;
} childObject;

/* Program global variables */
childObject *childObjects;
int childCount = 0, files = 0;
int checkArg = 0;
char *compilerType, *styleType;

/* void signal_kill(int signalType):
    Kill all remaining processes for SIGNINT
*/
void signal_kill(int signalType) {
    if(childObjects[childCount].processId != 0) {
        kill(childObjects[childCount].processId, SIGKILL);
    }
    exit(0);
}

/* void print_program_error(int error):
**      - Returns nothing
**      - Prints game error messages out to stderr(Standard error) given 
**      error code "error"
*/
void print_program_error(int error) {
    
    /* Switch on error type */
    switch(error) {
        case BAD_ARGUMENTS:
            fprintf(stderr, 
                    "Usage: thresher [--show] type command filename ...\n");
            break;
        case BAD_PARAM:
            fprintf(stderr, "Unknown build type\n");
            break;
        case EXEC_FAIL:
            fprintf(stderr, "Exec failed\n");
            break;
        case SYS_CALL_ERROR:
            fprintf(stderr, "System error\n");
    }
}

/* void printAnsi(int errorTypes[], char** errors, int numErrors, 
**       int childStatus, int index, int exitType):
**      - Prints ANSI errors to stdout after compiling.
*/
void printAnsi(int errorTypes[], char** errors, int numErrors, 
        int childStatus, int index, int exitType) {
    int i = 0;
    fprintf(stdout, "----\n");
    fflush(stdout);
    
    if(checkArg == 2) {
        /* Print out errors */
        for(i = 0; i < numErrors; i++) {
            fprintf(stdout, "%s", errors[i]);
        }
        fprintf(stdout, "----\n");
    }
    
    /* Print error summary */
    if(errorTypes[0] != 0 ) {
        fprintf(stdout, "%d implicit declaration\n", errorTypes[0]);
    }
    if(errorTypes[1] != 0 ) {
        fprintf(stdout, "%d undeclared\n", errorTypes[1]);
    }
    if(errorTypes[2] != 0 ) {
        fprintf(stdout, "%d c99\n", errorTypes[2]);
    }
    if(errorTypes[3] != 0 ) {
        fprintf(stdout, "%d c++ comment?\n", errorTypes[3]);
    }
    if(errorTypes[4] != 0 ) {
        fprintf(stdout, "%d other\n", errorTypes[4]);
    }
}

/* processAnsi(int childStatus, int index, int exitType):
**      - Takes the stderr stream from the ANSI C compiler and checks the 
**      stream for certain errors messages and keeps them in a record 
**      for printing.
*/
void processAnsi(int childStatus, int index, int exitType) {
    char outputString[1024];
    int errorTypes[5] = {0, 0, 0, 0, 0};
    int notFound = 0, lineNum = 0, errors = 0;
    char** fullErrors = (char**)malloc (sizeof(char*) * 1024);
    
    /* Read in from the stream while valid */
    while(fgets(outputString, 1024, childObjects[childCount-1].dataIn) 
            != NULL) {

        fullErrors[errors] = 
                (char*)malloc (sizeof(char) * strlen(outputString));
       
        strcpy(fullErrors[errors++], outputString);
        notFound = 0;
        
        if (strncmp(outputString, childObjects[childCount-1].filename, 
                strlen(childObjects[childCount-1].filename)) != 0) {
            continue;
        }
        
        /* Ensure there is a filename and line number */
        if(sscanf(outputString + strlen(childObjects[childCount-1].filename), 
                ":%d:", &lineNum) == 1) {
            
            /* Check ignored lines */
            if((strstr(outputString, "note:") == NULL) &&
                (strstr(outputString, "error: (Each undeclared") == NULL) &&
                (strstr(outputString, "error: for each function") == NULL)) {
                
                /* Check for errors */
                if(strstr(outputString, "implicit declaration") != NULL) {
                    errorTypes[0]++;
                    notFound = 1;
                }
                if((strstr(outputString, "undeclared") != NULL) 
                        && notFound == 0) {
                    errorTypes[1]++;
                    notFound = 1;
                }
                if((strstr(outputString, "C99 mode") != NULL) 
                        && notFound == 0) {
                    errorTypes[2]++;
                    notFound = 1;
                }
                if((strstr(outputString, 
                        "expected expression before '/' token") != NULL) 
                        && notFound == 0) {
                    errorTypes[3]++;
                    notFound = 1;
                }
                if(notFound == 0) {
                    errorTypes[4]++;
                }
            }
        }
    } 
    
    /* Print out */
    printAnsi(errorTypes, fullErrors, errors, childStatus, index, exitType);  
}

/* void printC(int errorTypes[], char** errors, int numErrors, 
**       int childStatus, int index, int exitType):
**      - Prints C99 errors to stdout after compiling.
*/
void printC(int errorTypes[], char** errors, int numErrors, int childStatus, 
        int index, int exitType) {
    int i = 0;
    fprintf(stdout, "----\n");
    fflush(stdout);
    if(checkArg == 2) {
        /* Print errors */
        for(i = 0; i < numErrors; i++) {
            fprintf(stdout, "%s", errors[i]);
        }
        fprintf(stdout, "----\n");
    }
    
    /* Print error summary */
    if(errorTypes[0] != 0 ) {
        fprintf(stdout, "%d implicit declaration\n", errorTypes[0]);
    }
    if(errorTypes[1] != 0 ) {
        fprintf(stdout, "%d undeclared\n", errorTypes[1]);
    }
    if(errorTypes[4] != 0 ) {
        fprintf(stdout, "%d other\n", errorTypes[4]);
    }
}

/* processC(int childStatus, int index, int exitType):
**      - Takes the stderr stream from the c99 compiler and checks the stream for
**      certain errors messages and keeps them in a record for printing.
*/
void processC(int childStatus, int index, int exitType) {
    char outputString[1024];
    int errorTypes[5] = {0, 0, 0, 0, 0};
    int notFound = 0, lineNum = 0, errors = 0;
  
    char** fullErrors = (char**)malloc (sizeof(char*) * 1024);
    
    /* Read in from the stream while valid */
    while(fgets(outputString, 1024, childObjects[childCount-1].dataIn) 
            != NULL) {
    
        fullErrors[errors] = 
                (char*)malloc (sizeof(char) * strlen(outputString));

        strcpy(fullErrors[errors++], outputString);
        notFound = 0;
        
        if (strncmp(outputString, childObjects[childCount-1].filename, 
                strlen(childObjects[childCount-1].filename)) != 0) {
            continue; 
        }
        
        /* Ensure there is a filename and line number */
        if(sscanf(outputString + strlen(childObjects[childCount-1].filename), 
                ":%d:", &lineNum) == 1) {
            
            /* Check ignored lines */
            if((strstr(outputString, "note:") == NULL) &&
                (strstr(outputString, "error: (Each undeclared") == NULL) &&
                (strstr(outputString, "error: for each function") == NULL)) {
                
                /* Check for errors */
                if(strstr(outputString, "implicit declaration") != NULL) {
                    errorTypes[0]++;
                    notFound = 1;
                }
                if((strstr(outputString, "undeclared") != NULL) 
                        && notFound == 0) {
                    errorTypes[1]++;
                    notFound = 1;
                }
                if((strstr(outputString, "C99 mode") != NULL) 
                        && notFound == 0) {
                    errorTypes[2]++;
                    notFound = 1;
                }
                if((strstr(outputString, 
                        "expected expression before '/' token") != NULL) 
                        && notFound == 0) {
                    errorTypes[3]++;
                    notFound = 1;
                }
                if(notFound == 0) {
                    errorTypes[4]++;
                }
            }
        }
    } 
    /* print out */
    printC(errorTypes, fullErrors, errors, childStatus, index, exitType); 
}

/* void printJava(int errorTypes[], char** errors, int numErrors, 
**       int childStatus, int index, int exitType):
**      - Prints Java errors to stdout after compiling.
*/
void printJava(int errorTypes[], char** errors, int numErrors, 
        int childStatus, int index, int exitType) {
    int i = 0;
    fprintf(stdout, "----\n");
    fflush(stdout);
    if(checkArg == 2) {
        /* Print errors */
        for(i = 0; i < numErrors; i++) {
            fprintf(stdout, "%s", errors[i]);
        }
        fprintf(stdout, "----\n");
    }
    /* Print error summary */
    if(errorTypes[0] != 0 ) {
        fprintf(stdout, "%d missing identifier\n", errorTypes[0]);
    }
    if(errorTypes[1] != 0 ) {
        fprintf(stdout, "%d missing symbol\n", errorTypes[1]);
    }
    if(errorTypes[2] != 0 ) {
        fprintf(stdout, "%d non-static access\n", errorTypes[2]);
    }
    if(errorTypes[3] != 0 ) {
        fprintf(stdout, "%d other\n", errorTypes[3]);
    }
}

/* processJava(int childStatus, int index, int exitType):
**      - Takes the stderr stream from the Java compiler and checks the 
**      stream for certain errors messages and keeps them in a record 
**      for printing.
*/
void processJava(int childStatus, int index, int exitType) {
    char outputString[1024];
    int errorTypes[4] = {0, 0, 0, 0};
    int notFound, lineNum, errors = 0;
   
    char** fullErrors = (char**)malloc (sizeof(char*) * 1024);
    /* Read in from the stream while valid */
    while(fgets(outputString, 1024, 
            childObjects[childCount-1].dataIn) != NULL) {
        fullErrors[errors] = 
                (char*)malloc (sizeof(char) * strlen(outputString));
        
        strcpy(fullErrors[errors++], outputString);
        notFound = 0;
        
        if (strncmp(outputString, childObjects[childCount-1].filename, 
                strlen(childObjects[childCount-1].filename)) != 0) {
            continue;
        }
        
        /* Ensure there is a filename and line number */
        if(sscanf(outputString + strlen(childObjects[childCount-1].filename), 
                ":%d:", &lineNum) == 1) {
                
            /* Check for errors */
            if(strstr(outputString, "<identifier> expected") != NULL) {
                errorTypes[0]++;
                notFound = 1;
            }
            if((strstr(outputString, "cannot find symbol") != NULL) 
                    && notFound == 0) {
                errorTypes[1]++;
                notFound = 1;
            }
            if((strstr(outputString, "static context") != NULL) 
                    && notFound == 0) {
                errorTypes[2]++;
                notFound = 1;
            }
            if(notFound == 0) {
                errorTypes[3]++;
            }
            
        }
    } 
    
    /* Print out */
    printJava(errorTypes, fullErrors, errors, childStatus, index, exitType); 
}

/* void printLatex(int errorTypes[], char** errors, int numErrors, 
**       int childStatus, int index, int exitType):
**      - Prints Latex errors to stdout after compiling.
*/
void printLatex(int errorTypes[], char** errors, int numErrors, 
        int childStatus, int index, int exitType) {
    int i = 0;
    fprintf(stdout, "----\n");
    fflush(stdout);
    if(checkArg == 2) {
        /* Prints errors */
        for(i = 0; i < numErrors; i++) {
            fprintf(stdout, "%s", errors[i]);
        }
        fprintf(stdout, "----\n");
    }
    
    /* Prints error summary */
    if(errorTypes[0] != 0 ) {
        fprintf(stdout, "%d math mode error\n", errorTypes[0]);
    }
    if(errorTypes[1] != 0 ) {
        fprintf(stdout, "%d bad macro\n", errorTypes[1]);
    }
    if(errorTypes[2] != 0 ) {
        fprintf(stdout, "%d warning\n", errorTypes[2]);
    }
    if(errorTypes[3] != 0 ) {
        fprintf(stdout, "%d error\n", errorTypes[3]);
    }
    if(errorTypes[4] != 0 ) {
        fprintf(stdout, "%d bad box\n", errorTypes[4]);
    }
}

/* processlatex(int childStatus, int index, int exitType):
**      - Takes the stdout stream from the Latex compiler and checks the 
**      stream for certain errors messages and keeps them in a record 
**      for printing. Also pushes data back into the stdin for compiling 
**      purposes.
*/
void processLatex(int childStatus, int index, int exitType) {
    char outputString[1024];
    int errorTypes[5] = {0, 0, 0, 0, 0};
    int notFound, errors = 0;
    char** fullErrors = (char**)malloc (sizeof(char*) * 1024);
    
    /* Read in from the stream while valid */
    while(fgets(outputString, 1024, 
            childObjects[childCount-1].dataIn) != NULL) {
        fullErrors[errors] = 
                (char*)malloc (sizeof(char) * strlen(outputString));
        strcpy(fullErrors[errors++], outputString);        
        notFound = 0;
        /*Check for errors*/
        if(strstr(outputString, "! Missing $ inserted.") != NULL) {
            /* Print \n back to stdin for compilation */
            fprintf(childObjects[childCount-1].dataOut, "\n");
            fflush(childObjects[childCount-1].dataOut);
            errorTypes[0]++;
            notFound = 1;
        }
        if((strstr(outputString, "! Undefined control sequence.") 
                != NULL) && notFound == 0) {   
            /* Print \n back to stdin for compilation */
            fprintf(childObjects[childCount-1].dataOut, "\n");
            fflush(childObjects[childCount-1].dataOut);
            errorTypes[1]++;
            notFound = 1;
        }
        if((strstr(outputString, "LaTeX Warning") != NULL) 
                && notFound == 0) {
            errorTypes[2]++;
            notFound = 1;
        }
        if((strstr(outputString, "LaTeX Error") != NULL) 
                && notFound == 0) {
            /* Print X\n back to stdin for compilation */
            fprintf(childObjects[childCount-1].dataOut, "X\n");
            fflush(childObjects[childCount-1].dataOut);
            errorTypes[3]++;
            notFound = 1;
        }
        if((strstr(outputString, "Overfull \\hbox") != NULL) 
                && notFound == 0) {
            errorTypes[4]++;
            notFound = 1;
        }
    } 
    printLatex(errorTypes, fullErrors, errors, childStatus, index, exitType);
}

/* compile(int argc, char** argv, int compileType, int index):
**      Compiles the given source file and the selected options.
**      Child program exit's with 3 if build problem.
*/
void compile(int argc, char** argv, int compileType, int index) {
    
    /* Switches on compiler style type */
    switch(compileType) {
        case ANSIC: 
            execlp(compilerType, compilerType, "-ansi", "-pedantic", "-Wall", 
                    childObjects[index].filename, NULL);      
            break;
        case C99:
            execlp(compilerType, compilerType, "-std=gnu99", "-pedantic", 
                    "-Wall", childObjects[index].filename, NULL);
            break;
        case JAVA:
            execlp(compilerType, compilerType, "-d", ".", 
                    childObjects[index].filename, NULL);
            break;
        case LATEX:
            execlp(compilerType, compilerType, 
                    childObjects[index].filename, NULL);
            break;
    }
    /* Exits with 3 if problem */
    exit(3);
}

/* void create_structs(int argc, char** argv, int compileType, int numProgram):
**      Main function for creating forks and managing pipes. Most of the 
**      program's work is done in this file.
*/
void create_structs(int argc, char** argv, int compileType, int numProgram) {
    
    /* Create file descriptors */
    int fd[2]; 
    int fd2[2]; 
    int pid = 0, childReturnValue = 0, child_status = 0, exitType = 0;
    
    /* Create a child(process) object*/
    childObject object;
    
    /* Try to create the pipes */
    if(pipe(fd) == -1) {
        print_program_error(SYS_CALL_ERROR);
        exit(SYS_CALL_ERROR);
    }
    if(pipe(fd2) == -1) {
        print_program_error(SYS_CALL_ERROR);
        exit(SYS_CALL_ERROR);
    }
    
    /* Set the object attibutes */
    object.running = 1;
    object.filename = argv[checkArg + 2  + numProgram];
    object.dataIn = fdopen(fd[0], "r");
    object.dataOut = fdopen(fd2[1], "w");
    object.pipeStatus = 1;
    object.compiler = argv[checkArg+1];
    object.fileType = compileType;
    
    /* Add the child object to the array of objects(processes */
    childObjects[childCount] = object;
 
    /* Fork */
    switch(pid = fork()) {
        case -1: /* Problem with fork */
            print_program_error(SYS_CALL_ERROR);
            exit(SYS_CALL_ERROR);
            
        case 0: /* Child */
            /*Close ends of pipes*/
            close(fd[0]);
            close(fd2[1]);
            
            /* If we are compiling latex, use stdout instead of stderr */
            if(compileType == LATEX) {
                if(dup2(fd[1], STDOUT_FILENO) == -1){
                    exit(SYS_CALL_ERROR);
                }
                if(dup2(fd2[0], STDIN_FILENO) == -1){
                    exit(SYS_CALL_ERROR);
                }
            } else {
                if(dup2(fd[1], STDERR_FILENO) == -1){
                    exit(SYS_CALL_ERROR);
                }
            }
            
            /* Call the compiler */
            compile(argc, argv, compileType, childCount);
            
            fclose(stdout);
            break;
        default: /* Parent */
            childObjects[childCount++].processId = pid;
            /* Close ends of pipes */
            close(fd[1]);
            close(fd2[0]);
            childReturnValue = 0;
            /* switch on filetype for processesing and printing */
            switch(childObjects[childCount-1].fileType) {
                case ANSIC: 
                    processAnsi(childReturnValue, childCount-1, exitType);
                    break;
                case C99:
                    processC(childReturnValue, childCount-1, exitType);
                    break;
                case JAVA:
                    processJava(childReturnValue, childCount-1, exitType);
                    break;
                case LATEX:
                    processLatex(childReturnValue, childCount-1, exitType);
                    break;
            }
            
            /* Wait for the program to die */
            waitpid(pid, &child_status, 0);
            
            /* Get the child's exit status */
            if (WIFEXITED(child_status)) {
                childReturnValue = WEXITSTATUS(child_status);
                exitType = 0;
            } else {
                exitType = 1;
            }
            
            /* Print file status here */
            if(childReturnValue != 3) { 
                if(exitType == 0) {
                    fprintf(stdout, "%s exited with status %d\n", 
                            childObjects[childCount-1].filename, 
                            childReturnValue);
                } else {
                    fprintf(stdout, "%s did not exit normally\n", 
                            childObjects[childCount-1].filename);
                }
            }
            
            fprintf(stdout, "----\n");
            
            /*End the program*/
            if(childReturnValue == 3) {
                print_program_error(EXEC_FAIL);
                exit(3);
            } else if(childReturnValue != 0) {
                exit(5);
            }         
    }   
}



/* void process_args(int argc, char** argv):
**          -returns build type or program error
**          -processes program arguments
*/
int process_args(int argc, char** argv) {
    
    /* Check if 1st argument is --show  */
    if(strcmp(argv[1], "--show") == 0) {
        if(argc > 4) {
            checkArg = 2;
        } else {
            print_program_error(BAD_ARGUMENTS);
            exit(BAD_ARGUMENTS);
        }
    }
    else checkArg = 1;
    
    files = argc - (checkArg + 2);
    
    /* Check for build type  */    
    if((strcmp(argv[checkArg], "ansiC") == 0)) {
        compilerType = argv[checkArg+1];
        return ANSIC;
    } else if(strcmp(argv[checkArg], "c99") == 0) {
        compilerType = argv[checkArg+1];
        return C99;
    } else if(strcmp(argv[checkArg], "java") == 0)  {
            compilerType = argv[checkArg+1];
        return JAVA;
    } else if(strcmp(argv[checkArg], "latex") == 0) {
        compilerType = argv[checkArg+1];
        return LATEX;
    } else {
        print_program_error(BAD_PARAM);
        exit(BAD_PARAM);
    }   
} 

int main(int argc, char** argv) { 
    int compileType, i = 0;
    
    struct sigaction sp;
    sp.sa_handler = signal_kill;
    sigaction(SIGINT, &sp, 0);
    signal(SIGINT, signal_kill);
    
    childObjects = (childObject*)malloc(sizeof(childObject) * 10);
    
    /* Verify the correct number of arguments  */
    if (argc > 3) {
        compileType = process_args(argc, argv);
        for(i = 0; i < files; i++) {
           create_structs(argc, argv, compileType, i);
        }
    } else {
        print_program_error(BAD_ARGUMENTS);
        exit(BAD_ARGUMENTS);
    }  
    return 0; 
}
