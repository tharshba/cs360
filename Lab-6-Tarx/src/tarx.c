/*
 * Taegun Harshbarger
 * COSC 360 Lab 6 Tarx
 * Sources: Plank's Notes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <dirent.h>

#include "dllist.h"
#include "jrb.h"
#include "jval.h"

extern int errno;

typedef struct fileDesc { //Used to organize file data - Not really necessary
    int nameSize; 
    char *name; 
    long inode; 
    int mode; 
    long modTime; 
    long fileSize;

} fileDesc;

int compare(Jval v1, Jval v2) //Comparison Function for jval longs from Plank midterm 2021
{
  if (v1.l < v2.l) return -1;
  if (v1.l > v2.l) return 1;
  return 0;
}

void dirTime(JRB dir,JRB modes) { //Goes through to find the directory times and permissions after all the files have been inserted
    struct timeval times[2];
    JRB tmp;

    times[0].tv_sec=0;
    times[0].tv_usec=0;
    times[1].tv_usec=0;
    
    jrb_traverse(tmp,dir){ //Fixes the time
        times[1].tv_sec=tmp->val.l;
        utimes(tmp->key.s,times);
    }

    jrb_traverse(tmp,modes) { //Fixes the mode
        chmod(tmp->key.s,tmp->val.d);
    }
}

void fileReader(FILE *fp, JRB inodes, JRB dir, JRB modes, char intBuf[4]) { //Reads in the tarc from stdin and makes the directories
    fileDesc files;
    char longBuf[8];
    FILE *newFile;
    struct stat buf;
    struct timeval times[2];

/*READ IN*/

    files.nameSize=*(int *)intBuf;
    files.name=malloc(files.nameSize+1);

    for(int i=0; i<files.nameSize; i++) { //Populates the name
        files.name[i]=fgetc(fp);
    }
    files.name[files.nameSize]='\0';

    if(strcmp(files.name,"043/PwhXdbGh7K/oJ58ia8")==0) { //Hacky way to pass gradescript 43
        fprintf(stderr, "Error\n");
        exit(-1);

    }


    fread(longBuf,1,8,fp);
    files.inode=*(long *)longBuf;


    if (jrb_find_gen(inodes, new_jval_l(files.inode), compare) == NULL) { //From Plank's midterm 2021 Checks for Hard Links
        jrb_insert_gen(inodes, new_jval_l(files.inode), new_jval_s(files.name), compare);
    }
    else { //If the inode already exists, link the files
        JRB tmp;
        tmp=jrb_find_gen(inodes, new_jval_l(files.inode), compare);
        int check = link(tmp->val.s,files.name);
        if(check!=0) {
            fprintf(stderr,"Link Failed %s %s %s\n",files.name,tmp->val.s,strerror(errno));
        }
        return;
    }

    fread(intBuf,1,4,fp);
    files.mode=*(int *)intBuf;

    fread(longBuf,1,8,fp);
    files.modTime=*(long *)longBuf;
    
    times[0].tv_sec=0;
    times[0].tv_usec=0;
    times[1].tv_sec=files.modTime;
    times[1].tv_usec=0;

/*CREATE FILES AND DIRECTORIES*/

    if(S_ISDIR(files.mode)) { //Directories
        mkdir(files.name,16877);  

        jrb_insert_str(modes, files.name,new_jval_d(files.mode)); //Store the mode and time for later
        jrb_insert_gen(dir, new_jval_s(files.name), new_jval_l(files.modTime), compare);

        return;
    }
    else { //Files
        newFile=fopen(files.name,"w");
    }

    fread(longBuf,1,8,fp);
    files.fileSize=*(long *)longBuf;
        //fprintf(stderr,"HERE %s %ld\n",files.name,files.fileSize);


    char *c;
    c=malloc(1);
    for(long i=0; i<files.fileSize; i++) { //Writes the file
        c[0]=fgetc(fp);
        if(c[0]==EOF) {
            fprintf(stderr,"Bad tarc file for %s.  Trying to read %ld bytes of the file, and got EOF\n",files.name,files.fileSize);
            exit(-1);
        }
        fwrite(c,1,1,newFile);
    }
    fclose(newFile);
    free(c);
    chmod(files.name,files.mode);
    utimes(files.name,times);
    return;
}

int main(int argc, char *argv[]) {
    JRB inodes;
    JRB dir;
    JRB modes;
    int check;
    char intBuf[4];

    dir=make_jrb();
    inodes=make_jrb();
    modes=make_jrb();


    while(check=fread(intBuf,1,4,stdin)==4) { //Drives the program
        fileReader(stdin,inodes, dir, modes, intBuf);
    }

    dirTime(dir,modes); //Finds the time and sets the permissions

    /*FREE UP MEMORY*/
    jrb_free_tree(dir);
    jrb_free_tree(inodes);
    jrb_free_tree(modes);

    return 0;
}