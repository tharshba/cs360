/*
 * Taegun Harshbarger
 * COSC 360
 * JShell
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/wait.h>

#include "jrb.h"
#include "fields.h"

void DupManager (int mode, char* fileName) {
    int fd1, fd2;

    if (mode == 1) { //Command needs to read from a file
        fd1 = open(fileName, O_RDONLY);
        if(fd1 < 0) {
            perror(fileName);
            exit(1);
        }
        if(dup2(fd1, 0) != 0) {
            perror("dup1");
            exit(1);
        }
        close(fd1);
    } else if (mode == 2) { //Command needs to create or/and write to a file
        fd2 = open(fileName, O_WRONLY | O_TRUNC | O_CREAT, 0666);
        if(fd2 < 0) {
            perror(fileName);
            exit(1);
        }
        if(dup2(fd2, 1) != 1) {
            perror("dup1");
            exit(1);
        }
        close(fd2);
    } else { //Commands needs to create or/and append to a file
        fd2 = open(fileName, O_WRONLY | O_APPEND | O_CREAT, 0666);
        if(fd2 < 0) {
            perror(fileName);
            exit(1);
        }
        if(dup2(fd2, 1) != 1) {
            perror("dup1");
            exit(1);
        }
        close(fd2);
    }
    return;
}

int forkhandler() {
    fflush(stdin);
    fflush(stdout);
    fflush(stderr);
    return(fork());
}

int main(int argc, char** argv) {

    char prompt[16];
    if (argc == 2) { //Sets the prompt depending on the input given
        if(argv[1][0] != '-') {
            strcpy(prompt, argv[1]);
        } else {
            prompt[0] = '\0';
        }
    } else {
        strcpy(prompt, "jsh1: ");
    }

    char input[550];
    for( ;; ) {
        int sizeCommand = 0, i, needsDup = 0, pipLen = 0;
        printf("%s", prompt);

        if(fgets(input, 550, stdin) == NULL || (strcmp(input, "exit\n") == 0)) { //Ends program if input is cntr-d or exit
            exit(1);
        }
        input[strlen(input) - 1] = '\0';
        char tmpInput[550];
        strcpy(tmpInput, input);

        for(i = 0; i < strlen(input); i++) { //Determines the number of arguments in the command
            if(input[i] == ' ') {
                sizeCommand++;
            }
        }
        sizeCommand++;
            
        char** newargv = NULL;
        char** finalargv = NULL;
        newargv = (char**) malloc(sizeof(char*) * (sizeCommand + 1));

        i = 0;
        char* ptr = strtok(tmpInput, " "); //Puts the command arguments into a char**
        while(ptr != NULL) {
            newargv[i] = ptr;
            if(strcmp(ptr, "<") == 0 || strcmp(ptr, ">") == 0 || strcmp(ptr, ">>") == 0) { //The command involved reading from or writing to files
                needsDup = 1;
            }

            if(strcmp(ptr, "|") == 0) { //The commands invovles pipes
                pipLen++;
            }
            i++;
            ptr = strtok(NULL, " ");
        }
        pipLen++;
        free(ptr);
        newargv[sizeCommand] = NULL;

        int pid, status;

        if (pipLen != 1) { //The command involves the use of pipes

            JRB allId, tmp;
            allId = make_jrb();

            int pipefd[2 * pipLen];
            for(i = 0; i < pipLen; i++) { //Makes the list of all the pipes beforehand
                pipe(pipefd + i * 2);
            }

            strcpy(tmpInput, input);
            ptr = strtok(tmpInput, "|");

            int j = 0;
            while(ptr) {
                if((pid = forkhandler()) == 0) {
                    if(strtok(NULL, "|") != NULL) { //If the command is not the very last command
                        if (dup2(pipefd[j + 1], 1) < 0) {
                            perror("dup2 1");
                            exit(1);
                        }
                    }

                    if (j != 0) { //If the command is not the very first command
                        if (dup2(pipefd[j - 2], 0) < 0) {
                            perror("dup2 2");
                            exit(1);
                        }
                    }

                    for(i = 0; i < 2 * pipLen; i++){ //Make sure all pipes are closed
                        close(pipefd[i]);
                    }

                    char newLine[550];
                    strcpy(newLine, (char*)ptr);

                    char* newPtr = strtok(newLine, " ");
                    finalargv = (char**) malloc(sizeof(char*) * sizeCommand);

                    i = 0;
                    while(newPtr != NULL) { //Makes the execute command include everything up to the first <, >, or >>
                        if (strcmp(newPtr, "<") != 0 && strcmp(newPtr, ">") != 0 && strcmp(newPtr, ">>") != 0) {
                            finalargv[i] = newPtr;
                        } else {
                            if(strcmp(newPtr, "<") == 0) { //There is a < in the command needing to open a file to read and use of fd 0
                                newPtr = strtok(NULL, " ");
                                DupManager(1, newPtr);
                            } else if (strcmp(newPtr, ">") == 0) { //There is a > in the command needing to open a file to write and use of fd 1
                                newPtr = strtok(NULL, " ");
                                DupManager(2, newPtr); 
                            } else { //There is a >> in the command needing to open a file to append and use of fd 1
                                newPtr = strtok(NULL, " ");
                                DupManager(3, newPtr);
                            }
                            break;
                        }
                        i++;
                        newPtr = strtok(NULL, " ");
                    }
                    finalargv[i] = NULL;

                    execvp(finalargv[0], finalargv); //Executes command and returns error if execvp returns
                    //perror(finalargv[0]);
                    //perror("here");
                    exit(1);
                } else {
                    jrb_insert_int(allId, pid, new_jval_d(pid)); //Inserts all the pids into a jrb tree
                }

                ptr = strtok(NULL, "|");
                j += 2;
            }
            
            for(i = 0; i < 2 * pipLen; i++){ //Makes sure all pipe file descriptors are closed
                close(pipefd[i]);
            }

            if(input[strlen(input) - 1] != '&') { 
                int returnId;
                while(!jrb_empty(allId)) { //Waits for all processes in the command to finish before going on
                    returnId = wait(&status);
                    if(finalargv != NULL) {
                        free(finalargv);
                    }

                    if((tmp = jrb_find_int(allId, returnId)) != NULL) {
                        jrb_delete_node(tmp);
                    }
                }
            }
            
            jrb_free_tree(allId);
            free(newargv);

        } else {
            
            if((pid = forkhandler()) == 0) {
                if(needsDup == 0) { //This is a child process and the command doesn't involve <, >, or >>
                    if(input[strlen(input) - 1] == '&') { //Removes the & from the command before execution
                        newargv[sizeCommand - 1] = NULL;
                    }

                    execvp(newargv[0], newargv); //Executes command and returns error if execup returns
                    perror(newargv[0]);
                    exit(1);
                } else if (needsDup != 0) { //This is a child process and the command contains either a <, >, or >>

                    for(i = 0; i < sizeCommand; i++) {
                        if(strcmp(newargv[i], "<") == 0) { //There is a < in the command needing to open a file to read and use of fd 0
                            DupManager(1, newargv[i + 1]);
                        } 
                        if (strcmp(newargv[i], ">") == 0) { //There is a > in the command needing to open a file to write and use of fd 1
                            DupManager(2, newargv[i + 1]);
                        }

                        if (strcmp(newargv[i], ">>") == 0) { //There is a >> in the command needing to open a file to append and use of fd 1
                            DupManager(3, newargv[i + 1]);
                        }
                    }

                    finalargv = (char**) malloc(sizeof(char*) * (sizeCommand));

                    i = 0;
                    while(1) { //Makes the execute command include everything up to the first <, >, or >>
                        finalargv[i] = newargv[i];
                        i++;
                        if(strcmp(newargv[i], "<") == 0 || strcmp(newargv[i], ">") == 0 || strcmp(newargv[i], ">>") == 0) {
                            break;
                        }
                    }
                    finalargv[i] = NULL;

                    execvp(finalargv[0], finalargv); //Executes command and returns error if execvp returns
                    perror(finalargv[0]);
                    exit(1);
                } 
            } else {
                //If the command doesn't end with an & then wait until wait returns the pid of the program fork() returned to eliminate zombie processes
                int pidd;
                if(input[strlen(input) - 1] != '&') { 
                    while(pidd != pid) {
                        pidd = wait(&status);
                    }
                }
            }
        }
    }
    return (0);
}
