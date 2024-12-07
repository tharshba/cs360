/*
 * Taegun Harshbarger
 * COSC 360 - Lab 4 - Fakemake
 * Sources: Dr. Plank's Notes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "fields.h"
#include "dllist.h"
#include "jval.h"

typedef struct DSFile { //Structure to help organize the file data
    Dllist cFiles; //C files
    Dllist hFiles; //H files
    Dllist oFiles; //O files
    Dllist flags; //Flags
    Dllist libraries; //Libraries
    char *executable; //Executable name
} DSFile;

int make_hfiles(DSFile *make_file) { //Checks the h files time and that they exist
    Dllist tmp;
    struct stat fileStat;
    int maxTime;

    dll_traverse(tmp, make_file->hFiles) { 
        if(stat(tmp->val.s, &fileStat)==0) {
            if(fileStat.st_mtime>maxTime) {
                maxTime=fileStat.st_mtime;
            }
        } 
        else {
			fprintf(stderr, "fmakefile: %s: No such file or directory\n", tmp->val.s);
			exit(-1);
		}
    }

    return maxTime;
}

int make_ofiles(DSFile *make_file) { //Checks the o files time and that they exist
    Dllist tmp;
    struct stat fileStat;
    int maxTime;

    dll_traverse(tmp, make_file->oFiles) { 
        if(stat(tmp->val.s, &fileStat)==0) {
            if(fileStat.st_mtime>maxTime) {
                maxTime=fileStat.st_mtime;
            }
        } 
        else {
			fprintf(stderr, "fmakefile: %s: No such file or directory\n", tmp->val.s);
			exit(-1);
		}
    }

    return maxTime;
}

int make_cfiles(DSFile *make_file, int maxTime) { //Creates the executable file
    char fileName[1000]; //Used to make .o file names
    struct stat fileStat;
    struct stat tmpStat;
    int numRedone=0; //Checks how many files needed to be changed
    Dllist tmp; //Used to traverse the list
    Dllist tmpTwo;
    

    dll_traverse(tmp,make_file->cFiles) { //Traverses all of the cfiles
        char gcc[1000]; //Used to create the command
        gcc[0]='\0';
        if(stat(tmp->val.s, &fileStat)==0) {
            fileName[0]='\0';
            strncat(fileName,tmp->val.s,(strlen(tmp->val.s)-1)); //Removes the c at the end of the file
            strcat(fileName, "o"); //Replaces the c with an o to make it executable
            dll_append(make_file->oFiles, new_jval_s(strdup(fileName))); //Adds the file to the ofiles list
            
            if(stat(fileName, &tmpStat)==0) {
                if(tmpStat.st_mtime < maxTime || tmpStat.st_mtime < fileStat.st_mtime) { //If the program's age is incorrect
                    numRedone++;
                    strcat(gcc,"gcc -c "); 
                    dll_traverse(tmpTwo,make_file->flags) { //Adds the flags to the command
                        strcat(gcc, tmpTwo->val.s);
                        strcat(gcc, " ");
                    }
                    strcat(gcc, tmp->val.s);
                    printf("%s\n",gcc);
                }
            }
            else {
                numRedone++;
                strcat(gcc,"gcc -c ");
                dll_traverse(tmpTwo,make_file->flags) { //Adds the flags to the command
                    strcat(gcc, tmpTwo->val.s);
                    strcat(gcc, " ");
                }
                strcat(gcc, tmp->val.s);
                printf("%s\n",gcc);
            }
            
            if(system(gcc)!=0) {
                fprintf(stderr, "Command failed.  Exiting\n");
                exit(-1);
            }
        }
        else {
			fprintf(stderr, "fmakefile: %s: No such file or directory\n", tmp->val.s);
			exit(-1);
		}
    }

    if(stat(make_file->executable, &fileStat)==0) { 
        int maxOTime = make_ofiles(make_file);
        if(maxOTime > fileStat.st_mtime) {
            numRedone++;
        }
    }
    else  {
        numRedone++;
    }

    if(numRedone!=0) { //If something has been redone, recompile
        char gcc[1000];
        gcc[0]='\0';
        strcat(gcc, "gcc -o ");

		strcat(gcc, make_file->executable); //Adds the executable name to the command line
		strcat(gcc, " ");

		dll_traverse(tmp, make_file->flags) { //Adds the flags to the command line
			strcat(gcc, tmp->val.s);
			strcat(gcc, " ");
		}

		dll_traverse(tmp, make_file->oFiles) { //Adds the files to the command line
			strcat(gcc, tmp->val.s);
			strcat(gcc, " ");
		}

		if (make_file->libraries != NULL) { 
			dll_traverse(tmp, make_file->libraries) { //Adds the libraries to the command line
				strcat(gcc, tmp->val.s);
				strcat(gcc, " ");
			}
		}
		gcc[strlen(gcc) - 1] = '\0'; //Null terminates the string
		printf("%s\n", gcc);

		if (system(gcc) != 0) {
			fprintf(stderr, "Command failed.  Fakemake exiting\n");
			exit(-1);
		}
    }

    return(numRedone);

}


int main(int argc, char *argv[]) {
    char *description_file_name="fmakefile";
    IS is;
    DSFile *make_file; //Stores all the info from the file
    
    if(argc==1) {
        is = new_inputstruct(description_file_name);
    }
    else if(argc==2){
        description_file_name=argv[1];
        is = new_inputstruct(description_file_name);
    }

    if (is==NULL) {
        fprintf(stderr,"%s No such file or directory\n",description_file_name);
        exit(-1);
    }

/*SET UP THE STRUCT FOR INPUTS*/
    make_file = malloc(sizeof(DSFile));
    make_file->cFiles = new_dllist();
    make_file->hFiles = new_dllist();
    make_file->oFiles = new_dllist();
    make_file->flags = new_dllist();
    make_file->libraries = new_dllist();


    int num_executable=0;

/*INPUT INTO STRUCT*/

    while(get_line(is)>=0) {
        if(is->NF!=0) {
            if(*is->fields[0] == 'C') {
                for(int i=1; i<is->NF; i++) {
                    dll_append(make_file->cFiles, new_jval_s(strdup(is->fields[i])));
                }
            }
            else if (*is->fields[0] == 'H') {
                for(int i=1; i<is->NF; i++) {
                    dll_append(make_file->hFiles, new_jval_s(strdup(is->fields[i])));
                }

            }
            else if (*is->fields[0] == 'F') {
                for(int i=1; i<is->NF; i++) {
                    dll_append(make_file->flags, new_jval_s(strdup(is->fields[i])));
                }

            }
            else if (*is->fields[0] == 'E') {
                num_executable++;
                if(num_executable>1) {
                    fprintf(stderr, "fmakefile (%d) cannot have more than one E line\n", is->line);
                    exit(-1);
                }
                make_file->executable=strdup(is->fields[1]);
            }
            else if (*is->fields[0] == 'L') {
                for(int i=1; i<is->NF; i++) {
                    dll_append(make_file->libraries, new_jval_s(strdup(is->fields[i])));
                }
            }
        }
    }

    if(num_executable==0) {
        fprintf(stderr, "No executable specified\n");
        exit(-1);
    }

/*COMPILES*/

    if(make_cfiles(make_file, make_hfiles(make_file))==0) { //Makes sure the make does nothing if everything is up to date
        printf("%s up to date\n",make_file->executable);
    }

/*FREE MEMORY*/

    jettison_inputstruct(is);
    free_dllist(make_file->cFiles);
    free_dllist(make_file->hFiles);
    free_dllist(make_file->oFiles);
    free_dllist(make_file->flags);
    free_dllist(make_file->libraries);
    free(make_file->executable);
    free(make_file);
    
    return 0;
}