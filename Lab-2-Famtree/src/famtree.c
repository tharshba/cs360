/* Taegun Harshbarger
 * Lab 2: FamTree
 * Sources: Dr. Plank's Notes and Wikipedia page for Topological Sort */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jrb.h"
#include "fields.h"
#include "dllist.h"
#include "jval.h"

typedef struct person /*Node struct from Dr. Plank's Notes*/
{
  char *name;
  char sex;
  struct person *father;
  struct person *mother;
  Dllist children;
  int visited; //Used in DFS
  int printed; //Makes sure no duplicates occur during the topological sort
} Person;

void cycleCheckDFS(Person *p,JRB people) {
    JRB buffer;
    buffer=jrb_find_str(people,p->name);
    p=buffer->val.v;
    if (p->visited == 1) { //Already visited
        return; 
    } 
	else if (p->visited == -1) { //Cycle
		 fprintf(stderr,"Bad input -- cycle in specification\n");
		 exit(-1);
	}
	p->visited = -1; //Sets this person as -1 to find later
    if(p->children!=NULL) {
        for (Dllist ptr = (p->children)->flink; ptr != (p->children); ptr = ptr->flink) { //Traverses the children
            Person *c = (Person *) ptr->val.v; 
            buffer=jrb_find_str(people,c->name);
            c=buffer->val.v;
            cycleCheckDFS(c,people);
        }
    } 
	p->visited = 1; //Sets the person as visited
}

int ifPrintable(Person *p) {//Checks to see if the person should be printed
    int fP,mP; //Used as booleans to tell if the mother and father were already printed
    /*If these ifs aren't used it seg faults*/
    if(p->mother == NULL) {  //No mother
        mP = 0;
    }
    else {
        mP = p->mother->printed;
    }
    if(p->father == NULL) { //No father
        fP = 0;
    }
    else {
        fP = p->father->printed;
    }

    //Seg Faults if a value is not set before
    if (p->father == NULL && p->mother == NULL) { //No father and no mother listed
        return 1;
    }
    if (fP == 1 && mP == 1) {//Father and mother are printed
        return 1;
    }
    if (fP == 1 && p->mother == NULL) { //Father is printed no mother
        return 1;
    }
    if (p->father == NULL && mP == 1) { //No father mother is printed
        return 1;
    }

    return 0; //Not ready to be printed
}

void TopPrint(JRB people, JRB buffer, Person *p, Dllist tmpNode,Dllist topologicalPrint) { //Prints out the Topological Sort
    p = (Person *) tmpNode->val.v;
    buffer=jrb_find_str(people,p->name);
    p=(Person *)buffer->val.v;
    printf("%s\n", p->name);
    p->printed = 1;
    if(p->sex=='M') {
        printf("  Sex: Male\n");
    }
    else if(p->sex=='F') {
        printf("  Sex: Female\n");
    }
    else {
        printf("  Sex: Unknown\n");
    }
    if(p->father!=NULL) {
        printf("  Father: %s\n",p->father->name);
    }
    else if(p->father==NULL) {
        printf("  Father: Unknown\n");
    }
    if(p->mother!=NULL) {
        printf("  Mother: %s\n",p->mother->name);
    }
    else if(p->mother==NULL) {
        printf("  Mother: Unknown\n");
    }
    if(p->children!=NULL) {
        printf("  Children:\n");
        for (Dllist ptr = (p->children)->flink; ptr != (p->children); ptr = ptr->flink) {
            Person *bufferNode=ptr->val.v;
            buffer=jrb_find_str(people,bufferNode->name);
            bufferNode=(Person *)buffer->val.v;
            printf("    %s\n",bufferNode->name);
            dll_append(topologicalPrint,new_jval_v(bufferNode));
        }
    }
    else {
        printf("  Children: None\n");
    }
    printf("\n");
}

/* MAIN PROGRAM BEGINS HERE*/

int main(int argc, char *argv[]) {
    JRB people, tmp, buffer; //people is the actual JRB. tmp and buffer are used to copy for specific nodes
    IS is; //Input Structure
    Person *p; //Person node
    Person *bufferNode; //Copy
    int nsize, i;//Used in input
    int lineCount; //Used in the error checking
    Person *lastp; //Used to find the name the last time "PERSON" was stated
    Dllist topologicalPrint, tmpNode; //Dllists to print out in topological order
    Person *tmpP; //tmp person

    is = new_inputstruct(NULL);
    people = make_jrb();

    while (get_line(is) >= 0) { //Input into the structs and tree || This is an absolute mess, but it works... For the most part
        lineCount++;
        if (is->NF > 1) {
            p = malloc(sizeof(Person));
            nsize = strlen(is->fields[1]);
            
            for (i = 0; i < is->NF-1; i++) nsize += (strlen(is->fields[i])+1);

            p->name = (char *) malloc(sizeof(char)*(nsize+1)); //Allocates memory to the size of the name

            strcpy(p->name, is->fields[1]); //Copies the first part of the name into the struct
            for(i=2; i<is->NF; i++) { //Concatenates the rest of the into the string
                strcat(p->name, " ");
                strcat(p->name, is->fields[i]); 
            }
            /*PERSON*/
            if(strcmp(is->fields[0],"PERSON")==0) {         
                if(jrb_find_str(people,p->name)==NULL){ //Checks to see if the person is already in the tree
                    jrb_insert_str(people, p->name, new_jval_v((void *) p)); //Inserts the names into the JRB
                }
                lastp=p;//Stores the last time person was called
                buffer=jrb_find_str(people,p->name);
            }
            /*FATHER_OF*/
            else if(strcmp(is->fields[0],"FATHER_OF")==0) { //Sets the sex of a father and inserts their children
                buffer=jrb_find_str(people,lastp->name);
                bufferNode=(Person *)buffer->val.v;
                if(bufferNode->sex=='F') {
                    fprintf(stderr,"Bad input - sex mismatch on line %d\n",lineCount);
                    exit(-1);
                }
                bufferNode->sex='M';
                if(jrb_find_str(people,p->name)==NULL) {
                    p->father=bufferNode;
                    jrb_insert_str(people, p->name, new_jval_v((void *) p));
                }
                if(p->father==NULL) {
                    buffer=jrb_find_str(people,p->name);
                    bufferNode=(Person *)buffer->val.v;
                    bufferNode->father=bufferNode;
                    Person *father;
                    buffer=jrb_find_str(people,lastp->name);
                    father=(Person *)buffer->val.v;
                    bufferNode->father=father;
                }
                
                buffer=jrb_find_str(people,lastp->name);
                bufferNode=(Person *)buffer->val.v;
                if(bufferNode->children==NULL) {
                    bufferNode->children=new_dllist();
                    dll_append(bufferNode->children,new_jval_v((void *) p));
                }
                else {
                    dll_append(bufferNode->children,new_jval_v((void *) p));
                }
            }
            /*MOTHER_OF*/
            else if(strcmp(is->fields[0],"MOTHER_OF")==0) { //Sets the sex of a mother and inserts their children
                buffer=jrb_find_str(people,lastp->name);
                bufferNode=(Person *)buffer->val.v; 
                if(bufferNode->sex=='M') {
                    fprintf(stderr,"Bad input - sex mismatch on line %d\n",lineCount);
                    exit(-1);
                }
                bufferNode->sex='F';
                if(jrb_find_str(people,p->name)==NULL) {
                    p->mother=bufferNode;
                    jrb_insert_str(people, p->name, new_jval_v((void *) p));
                }
                if(p->mother==NULL) {
                    Person *bufferNode;
                    buffer=jrb_find_str(people,p->name);
                    bufferNode=(Person *)buffer->val.v;
                    Person *mother;
                    buffer=jrb_find_str(people,lastp->name);
                    mother=(Person *)buffer->val.v;
                    bufferNode->mother=mother;
                }
                Person *bufferNode; //Sets children
                buffer=jrb_find_str(people,lastp->name);
                bufferNode=(Person *)buffer->val.v;
                if(bufferNode->children==NULL) {
                    bufferNode->children=new_dllist();
                    dll_append(bufferNode->children,new_jval_v((void *) p));
                }
                else {
                    dll_append(bufferNode->children,new_jval_v((void *) p));
                }
            } 
            /*SEX*/
            else if(strcmp(is->fields[0],"SEX")==0) { //Sets up the sexes of the fam
                if (is->fields[1][0]=='M') {
                    buffer=jrb_find_str(people,lastp->name);
                    lastp=(Person *)buffer->val.v;
                    if(lastp->sex=='F') {
                        fprintf(stderr,"Bad input - sex mismatch on line %d\n",lineCount);
                        exit(-1);
                    }
                    lastp->sex='M';
                }
                else if (is->fields[1][0]=='F') { 
                    buffer=jrb_find_str(people,lastp->name); 
                    lastp=(Person *)buffer->val.v;
                    if(lastp->sex=='M') {
                        fprintf(stderr,"Bad input - sex mismatch on line %d\n",lineCount);
                        exit(-1);
                    }
                    lastp->sex='F';
                }
            }
            /*FATHER*/
            else if(strcmp(is->fields[0],"FATHER")==0) { //Sets the father and sets this as a child
                buffer=jrb_find_str(people,lastp->name); 
                bufferNode=(Person *)buffer->val.v;
                
                if(jrb_find_str(people,p->name)==NULL) { //If it was not already in the tree the father is added
                    p->children=new_dllist();
                    p->sex='M';
                    dll_append(p->children,new_jval_v((void *) bufferNode));
                    jrb_insert_str(people, p->name, new_jval_v((void *) p)); //Inserts the names into the JRB
                }
                else {
                    buffer=jrb_find_str(people,p->name);
                    p=(Person *)buffer->val.v;
                    if (p->children==NULL) {
                        p->children=new_dllist();
                    }
                    dll_append(p->children,new_jval_v((void *) bufferNode));
                    if(p->sex=='F') {
                        fprintf(stderr,"Bad input - sex mismatch on line %d\n",lineCount);
                        exit(-1);
                    }
                    p->sex='M';
                }
                if(bufferNode->father!=NULL && bufferNode->father!=p) {
                    fprintf(stderr,"Bad input -- child with two fathers on line %d\n",lineCount);
                    exit(-1);
                }
                bufferNode->father=p;
            }
            /*MOTHER*/
            else if(strcmp(is->fields[0],"MOTHER")==0) { //Sets the mother and sets this as a child 
                buffer=jrb_find_str(people,lastp->name); 
                bufferNode=(Person *)buffer->val.v;
                if(jrb_find_str(people,p->name)==NULL) { //If it was not already added the mother is added
                    p->children=new_dllist();
                    p->sex='F';
                    dll_append(p->children,new_jval_v((void *) bufferNode));
                    jrb_insert_str(people, p->name, new_jval_v((void *) p)); //Inserts the names into the JRB
                }
                else {
                    buffer=jrb_find_str(people,p->name);
                    p=(Person *)buffer->val.v;
                    if (p->children==NULL) {
                        p->children=new_dllist();
                    }
                    dll_append(p->children,new_jval_v((void *) bufferNode));
                    if(p->sex=='M') {
                        fprintf(stderr,"Bad input - sex mismatch on line %d\n",lineCount);
                        exit(-1);
                    }
                    p->sex='F';
                }
                if(bufferNode->mother!=NULL && bufferNode->mother!=p) {
                    fprintf(stderr,"Bad input -- child with two mothers on line %d\n",lineCount);
                    exit(-1);
                }
                bufferNode->mother=p;
            }
        }
    }// End of the input and mess

/*CYCLE CHECK*/
    jrb_traverse(tmp,people) { //Checks for cycles
        p=(Person *)tmp->val.v;
        cycleCheckDFS(p,people);
    }

/*TOPOLOGICAL SORT*/

    topologicalPrint=new_dllist();

    jrb_traverse(tmp,people) { //Adds the nodes without a mother and gather to the list
        p = (Person *) tmp->val.v;
        if(p->father==NULL && p->mother==NULL) {
            dll_append(topologicalPrint, new_jval_v(p));
        }
    }

	while((dll_empty(topologicalPrint))!=1){ //Goes through the doubly linked list until it is empty
		
		tmpP = (Person *) dll_first(topologicalPrint)->val.v; //Finds the actual pointer to that value
        buffer=jrb_find_str(people,tmpP->name);
        tmpP=(Person *) buffer->val.v;    
		
        if(tmpP->printed == 0){ //If the node is not already printed. Makes sure there are no duplicates printed
			if(ifPrintable(tmpP)) { //Finds if the node should be printed or not by checking whether parents are printed
                TopPrint(people,buffer,p,dll_first(topologicalPrint),topologicalPrint); //Prints the nodes sorted topologically
            }
        } 
        dll_delete_node(dll_first(topologicalPrint));//Deletes the node
    }

/*TOPOLOGICAL SORT ENDS*/

    //Attempts to free up memory and pass valgrind
    jrb_traverse(tmp,people) {
        p = (Person *) tmp->val.v;
        free(p->name);
        if(p->children!=NULL) {
            free_dllist(p->children);
        }
        free(p);
    }
    //free_dllist(topologicalPrint); Seg faults if I do this????
    jrb_free_tree(people);
    jettison_inputstruct(is); 
    return 0;
}