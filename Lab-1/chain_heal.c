/*
Name: Taegun Harshbarger
Lab 1: Chain Heal
Resources: Dr. Plank's Notes
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>           /* If you include this, you need to compile with -lm */

typedef struct node /*Node struct from Dr. Plank's Notes*/
{
  char name[100];
  int x, y;
  int cur_PP, max_PP;
  struct node *prev;
  int adj_size;
  struct node **adj;
  int visited;
  int healing;
} Node;

typedef struct healingVars //Structure that holds all of the input healing values
{
  int initial_range, jump_range, num_jumps, initial_power, best_path_length, best_healing;
  int *healing;
  Node **best_path;
  double power_reduction;
} healingVars;

double distanceFormula(int x1, int y1,int x2, int y2) { //Calculates the distance between players
  double ans;
  ans=sqrt(pow((x2-x1),2)+pow((y2-y1),2));
  return(ans);
}

void DFS(Node *player, int hopNum, healingVars *vars, int total_healing) { //Runs the DFS
  if(player->visited==1) {
    return;
  }

  int currHealing=rint((vars->initial_power)*pow((1-vars->power_reduction),hopNum));
  
  if(player->cur_PP+currHealing<player->max_PP) { //If max_PP is not hit
    total_healing=total_healing+currHealing;
    player->healing=currHealing;
  }
  else {
    player->healing=player->max_PP-player->cur_PP;
    total_healing=total_healing+player->healing; //If max_PP is hit
  }

  if(vars->best_healing<total_healing) { //Recalculates total_healing if there is a better path
    vars->best_path_length=hopNum+1;
    vars->best_healing=total_healing;
    Node *buffer=player;

    for(int i=0; i<vars->best_path_length; i++) { //Iterates through the current best path
      vars->best_path[i]=buffer;
      vars->healing[i]=buffer->healing;
      buffer=buffer->prev;   
    }
  }

  player->visited=1;
  
  for(int i=0; i<player->adj_size; i++) {
    if (player->adj[i]->visited==0 && hopNum+1<vars->num_jumps) {
      player->adj[i]->prev=player;
      DFS(player->adj[i],hopNum+1,vars,total_healing);
    }
  }
    player->visited=0;
}


int main(int argc, char *argv[]) {
  healingVars vars; //Stores the healing character variables

  vars.initial_range=atoi(argv[1]); 
  vars.jump_range=atoi(argv[2]);
  vars.num_jumps=atoi(argv[3]);
  vars.initial_power=atoi(argv[4]);
  vars.power_reduction=atof(argv[5]);

  Node *players; //Holds all of the players and their values
  players=(Node*)malloc(sizeof(Node));
  int numPlayers=0; //Keeps track of the number of players
  
  while(scanf("%d",&players[numPlayers].x)>0) { //Reads in the players
    scanf("%d",&players[numPlayers].y);
    scanf("%d",&players[numPlayers].cur_PP);
    scanf("%d",&players[numPlayers].max_PP);
    scanf("%s",players[numPlayers].name);
    
    players=realloc(players,((numPlayers+1)*2)*sizeof(Node)); //Definitely way too much memory but it works
    numPlayers++;
  }

  double *dist;
  dist=(double*)malloc(sizeof(double)*numPlayers*numPlayers); //Allocates memory to an array storing the distance from each player

  int k=0;
  for(int i=0; i<numPlayers; i++) { //Finds the distance b/n all players
    for(int j=0; j<numPlayers; j++) {
      dist[k]=distanceFormula(players[i].x,players[i].y,players[j].x,players[j].y);//k=width*row+col
      if(dist[k]<=vars.jump_range && i!=j) {
        players[i].adj_size++;
      }
      k++;
    }
  }

  for(int i=0; i<numPlayers; i++) {
    players[i].adj=(Node**)malloc(players[i].adj_size*sizeof(Node));
  }

  k=0;
  for(int i=0; i<numPlayers; i++) { //Uses the same parameters to set up the adj list
    int q=0;
    for(int j=0; j<numPlayers; j++) {
      if(dist[k]<=vars.jump_range && i!=j) { 
        players[i].adj[q]=&players[j]; //Adj List for the rest of the nodes
        q++;
      }
      k++;
    }
  }

int total_healing=0;

vars.healing=(int*)malloc(sizeof(int)*vars.num_jumps);
vars.best_path=(Node**)malloc(sizeof(Node)*vars.num_jumps);

for(int i=0; i<numPlayers; i++) { //Goes through all of the nodes within Urgosa's initial range
  if(dist[i]<=vars.initial_range) {
    DFS(players+i,0,&vars,total_healing);
  }
}

for(int i=vars.best_path_length-1; i>=0; i--) { //Prints out the best path
  printf("%s %d\n",vars.best_path[i]->name,vars.healing[i]);
}

printf("Total_Healing %d\n",vars.best_healing); //Prints out total healing

free(vars.best_path); //A very poor attempt at freeing up the memory. This program has leaks everywhere
free(vars.healing);
free(players);
free(dist);

for(int i=0; i<numPlayers; i++) {
  free(players[i].adj);
}

  return 0;
}