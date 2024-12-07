/*Taegun Harshbarger
 * Lab 3:Huffman Decoder
 * Sources: Dr. Plank's Notes */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


struct huff_node {
  struct huff_node *zero;
  struct huff_node *one;
  char *value;
};

struct huff_node* newNode() {
  struct huff_node *node=malloc(sizeof(struct huff_node));
  node->value=NULL;
  node->one=NULL;
  node->zero=NULL;
  return(node);
}

int lastFourCalc(int lastFour[8]) {
  return((lastFour[0]*268435456)+(lastFour[1]*16777216)+(lastFour[2]*1048576)+(lastFour[3]*65536)+(lastFour[4]*4096)+(lastFour[5]*256)+(lastFour[6]*16)+(lastFour[7]));
}

int myCeil(int t) {
 return(t/8 + (t%8 != 0));
}

int main(int argc, char *argv[]) {
  if (argc!=3) {
      fprintf(stderr,"Wrong Input\n");
  }

  FILE *defineFile;
  FILE *inputFile;
  int sizeOfDefine=0;
  int sizeOfInput=0;
  int buffer;
  unsigned int defineStr[1000000];
  unsigned int inputStr[1000000];
  struct huff_node *definition;


  defineFile=fopen(argv[1],"r");
  inputFile=fopen(argv[2],"r");

  while((buffer=fgetc(defineFile))!=EOF){ //Reads the define file into an array of ints 
    sizeOfDefine++;
    //defineStr=malloc(sizeOfDefine);
    defineStr[sizeOfDefine-1]=buffer;
    //fread(defineStr,sizeof(int),1,defineFile);
  }
/*  for(int i=0; i<sizeOfDefine; i++) {
    printf("Define:%d\n",defineStr[i]);
  }*/
  fclose(defineFile);

  while((buffer=fgetc(inputFile))!=EOF){ //Reads the input file into an array of ints 
    sizeOfInput++;
    //inputStr=malloc(sizeOfInput);
    inputStr[sizeOfInput-1]=buffer;
    //fread(defineStr,sizeof(int),1,inputFile);
  }/* 
  for(int i=0; i<sizeOfInput; i++) {
    printf("Input:%x %d\n",inputStr[i],inputStr[i]); 
  }*/
  fclose(inputFile);
  if(sizeOfInput<4) { //Error checks to see if the file is more than four bytes
    fprintf(stderr,"Error: file is not the correct size.\n");
    exit(-1);
  }



  struct huff_node *root; //Beginning node
  //definition=malloc(sizeof(struct huff_node)*sizeOfDefine);
  definition=newNode();
  root=definition;

  //bufferStr=malloc(sizeof(char)*(sizeOfDefine));

  int i=0;
  while (i<sizeOfDefine) { //Goes through the define file and makes the binary tree
    int j=0;
    char bufferStr[10000];
    while(defineStr[i]!='\0') {
      bufferStr[j]=defineStr[i];
      j++;
      i++;
    }
/*    i=i-j;
    bufferStr=malloc(sizeof(char)*j);
    for(int k=0;k<j;k++) {
      bufferStr[k]=defineStr[i];
      i++;

    }*/
    i++;
    //printf("%d %d %s\n",i,j,bufferStr);
    while(defineStr[i]!='\0') {
      if(defineStr[i]=='0') {
        if(definition->zero!=NULL) {
        definition=definition->zero;
        }
        else {
          definition->zero=newNode();
          definition=definition->zero;
        }
        //printf("0");
      }
      else if(defineStr[i]=='1') {
        if(definition->one!=NULL) {
        definition=definition->one;
        }
        else {
          definition->one=newNode();
          definition=definition->one;
        }
        //printf("1");
      }
      i++;
    }
    definition->value=malloc(sizeof(char)*j+1);
    strcpy(definition->value,bufferStr);
    memset(bufferStr,0,strlen(bufferStr));
//    printf("%s\n",definition->value);
    definition=root;
    i++;

  }

/*  definition=root;
  definition=definition->zero;
  definition=definition->one;
  definition=definition->one;
  printf("%s\n",definition->value);*/

 //int binary[40000];
 int *binary=malloc(sizeof(int)*(sizeOfInput-4)*8);
 int k=0;

  int j=1;
  //printf("%d",sizeOfInput);
  int lastFour[8];
  for(int i=0; i<8; i++) { //Finds the last four bytes || I should have just used fseek()
    if(inputStr[sizeOfInput-j]>16) {
      int ones=inputStr[sizeOfInput-j]%16;
      int tens=((inputStr[sizeOfInput-j])/16);
      lastFour[i]=tens;
//      printf("%d\n",lastFour[i]);
      i++;
      lastFour[i]=ones;
//      printf("%d\n",lastFour[i]);
      j++;
    }
    else if(inputStr[sizeOfInput-j]%10==inputStr[sizeOfInput-j] && inputStr[sizeOfInput-j]!=0) {
      lastFour[i]=0;
      i++;
//      printf("%d\n",lastFour[i]);
      lastFour[i]=inputStr[sizeOfInput-j];
//      printf("%d\n",lastFour[i]);
      j++;
    }
    else if(inputStr[sizeOfInput-j]==0) {
      lastFour[i]=0;
//      printf("%d\n",lastFour[i]);
      i++;
      lastFour[i]=0;
//      printf("%d\n",lastFour[i]);
      j++;
    }
    else {
      lastFour[i]=0;
//      printf("%d\n",lastFour[i]);
      i++;
      lastFour[i]=inputStr[sizeOfInput-j];
      j++;
//      printf("%d\n",lastFour[i]);
    }
  }
  int a[8];  
  j=0;
  int t=0;
  int tB=0;
  tB=lastFourCalc(lastFour);
//  printf("%d %d\n",tB,sizeOfInput);

  t=myCeil(tB);
  
  if(sizeOfInput-4!=t) { //Error checks to see if the input file is larger than it is supposed to be
    fprintf(stderr,"Error: Total bits = %d, but file's size is %d\n",tB,sizeOfInput);
    exit(-1);
  }


//  printf("%d %d\n",t,tB);

  while(k<t) { //Reads through the input file and converts it to binary
    i=0;  
    for(i=0;i<8;i++) {  
  //      printf("%d\n",inputStr[k]);
      a[i]=inputStr[k]%2;
      binary[j]=a[i];    
      inputStr[k]=inputStr[k]/2;  
      j++;
    }
    k++;
  }

    for(int j=0; j<(tB); j++) { //Goes through the input file and prints the decoded language
      if(binary[j]==0) {
        if (definition->zero!=NULL) {
          definition=definition->zero;
        }
        else{
          if(definition->value==NULL) {
            fprintf(stderr,"Unrecognized bits\n");
            exit(-1);
          }
          printf("%s",definition->value);
          definition=root;
          j--;
        }
      }
      else if(binary[j]==1){
        if (definition->one!=NULL) {
          definition=definition->one;
        }
        else{
          if(definition->value==NULL) {
            fprintf(stderr,"Unrecognized bits\n");
            exit(-1);
          }
          printf("%s",definition->value);
          definition=root;
          j--;
        }
      }
    }
      if(definition->value!=NULL) { //Prints the last value if needed
        printf("%s",definition->value);
      }
  

  
  free(binary);

  return(0);
}