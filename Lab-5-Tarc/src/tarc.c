/*
 * Taegun Harshbarger
 * Lab 5 Tarc
 * Sources: Dr. Plank's Notes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include "dllist.h"
#include "jrb.h"
#include "jval.h"

int compare(Jval v1, Jval v2) //Comparison Function for jval longs from Plank midterm 2021
{
  if (v1.l < v2.l) return -1;
  if (v1.l > v2.l) return 1;
  return 0;
}

void littleEndianInt(int var) { //Print for if the value is an int

    printf("%c%c%c%c",var & 0xFF, (var>>8) & 0xFF, (var>>16) & 0xFF, (var>>24) & 0xFF);

    return;
}

void littleEndianLong(unsigned long var) { //Print for if the value is a long

    littleEndianInt(var);
    printf("%c%c%c%c",(var>>32) & 0xFF, (var>>40) & 0xFF, (var>>48) & 0xFF,(var>>56) & 0xFF);

    return;
}

void fileTar(char *fileName,char *dirName, JRB inodes, int start) { //Prints the information
    char *actualFileName;
    struct stat buf;

    stat(fileName,&buf);

    if(start==0) {
        littleEndianInt(3); //Prints the size of the file
        actualFileName=strdup(fileName+(strlen(fileName)-3));

    }
    else {
        littleEndianInt(strlen(fileName)-strlen(dirName)+3); //Prints the size of the file
        actualFileName=strdup(fileName+(strlen(dirName)-3)); //Finds the file name without the path
    }

    for(int i=0; i<strlen(actualFileName); i++) {
        printf("%c",actualFileName[i]); //Prints file name in hex
    }

    if (jrb_find_gen(inodes, new_jval_l(buf.st_ino), compare) == NULL) { //From Plank's midterm 2021
        jrb_insert_gen(inodes, new_jval_l(buf.st_ino), new_jval_i(0), compare);
        littleEndianLong(buf.st_ino); //Prints the inode if it is a new inode
    }
    else {
        littleEndianLong(buf.st_ino); //Prints the inode and ends the procedure if it already exists
        return;
    }
    
    littleEndianInt(buf.st_mode); //Prints the mode
    
    littleEndianLong(buf.st_mtime); //Prints the time

    if(S_ISDIR(buf.st_mode)) { //If file is a directory stop procedure
        return;
    }

    littleEndianLong(buf.st_size); //Prints the size

    FILE *f;
    char c;

    f=fopen(fileName,"r");
    
    if (f == NULL)
    {
        fprintf(stderr,"Cannot open file %s\n");
        exit(-1);
    }

    c = fgetc(f);
    while (c != EOF) //Prints the contents of the file
    {
        printf ("%c", c);
        c = fgetc(f);
    }
  
    fclose(f);

    free(actualFileName);
    return;

}

void traverseDir(char *directoryName,char *startDirName,JRB inodes) { //Traverses the directories
    DIR *d;
    Dllist directories, tmp;
    struct dirent *de;
    struct stat buf;
    int exists;
    char* fileName;

    directories=new_dllist();

    d=opendir(directoryName);
    if(d==NULL) {
        fprintf(stderr, "Couldn't open \"%s\"\n",directoryName);
        exit(-1);
    }

    /*BUILDS THE FILE PATHS AND APPENDS DIRECTORIES TO THE DLLIST*/

    for(de=readdir(d); de!=NULL; de=readdir(d)) { //Traverses the directory
        if(strcmp(de->d_name,".")!=0 && strcmp(de->d_name,"..")!=0) { //Makes sure loops do not occur
            int fileLength=strlen(directoryName)+strlen(de->d_name);
            fileName=malloc(fileLength+2);
            fileName=strcpy(fileName,directoryName);
            strcat(fileName,"/");
            strcat(fileName,de->d_name);
            exists=stat(fileName, &buf); 
            if(exists!=0) {
                fprintf(stderr, "%s not found\n", fileName);
            }
            else {
                if(S_ISDIR(buf.st_mode)) { 
                    dll_append(directories, new_jval_s(strdup(fileName)));
                }
                else {
                    fileTar(fileName,startDirName,inodes,1);
                }
            strcpy(fileName,"");
            }
        }
    }
    closedir(d);//Closes the directories

    /*RECURSIVELY GOES THROUGH DIRECTORIES BY USING THE DLLIST*/

    dll_traverse(tmp,directories) { //Avoids having too many files open
        fileTar(tmp->val.s,startDirName,inodes,1);
        traverseDir(tmp->val.s,startDirName,inodes);
    }


    /*FREE MEMORY*/
    dll_traverse(tmp, directories) free(tmp->val.s);
    free_dllist(directories);
    return;
}


int main(int argc, char *argv[]) {
    DIR *d;
    struct dirent *de;
    struct stat buf;
    JRB inodes;
    int exists;
    char* fileName;

    if(argc!=2) {
        exit(-1);
    }

    d=opendir(argv[1]);
    if(d==NULL) {
        fprintf(stderr, "Couldn't open \"%s\"\n",argv[1]);
        exit(-1);
    }

    inodes=make_jrb(); //JRB tree to check inodes
    exists=stat(argv[1], &buf); 
    if(exists<0) {
        fprintf(stderr, "%s not found\n", argv[1]);
    }

    fileTar(argv[1],argv[1],inodes,0); //Prints the original directory

    traverseDir(argv[1],argv[1],inodes); //Drives the rest of the program

    closedir(d);
    jrb_free_tree(inodes);

    return 0;
}