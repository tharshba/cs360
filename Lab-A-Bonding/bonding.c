/*
 * Taegun Harshbarger
 * Lab A: Bonding Lab
 * Sources: Dr. Plank's Notes
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <dllist.h>
#include "bonding.h" //Provided by Dr. Plank

typedef struct thread { //Thread data structure as defined in the lab notes
  int h1;                 //Copied from Plank's lab notes -- Hydrogen 1
  int h2;                 //Copied from Plank's lab notes -- Hydrogen 2
  int o;                  //Copied from Plank's lab notes -- Oxygen
  int threadID;
  pthread_cond_t *cond;
}thread;

typedef struct global_info { //Global struct as defined in the lab notes
  Dllist wHydro;        //Waiting Hydrogen atoms
  Dllist wOxygen;       //Waiting Oxygen atoms
  pthread_mutex_t *lock;  

}global_info;


void *initialize_v(char *verbosity) { //Basically copied from the lab notes with a few changes
  global_info *g;  

  g = ( global_info *) malloc(sizeof( global_info));
  g->wHydro = new_dllist();
  g->wOxygen = new_dllist();
  g->lock = new_mutex();

  return (void *) g;
}

void *hydrogen(void *arg) { 
  struct bonding_arg *a; //From Lab Notes        
  global_info *g; //From Lab Notes
  char *rv; //From Lab Notes 
  thread *t, *nt1, *nt2;  //New threads that the hydrogen will bond to -- t is hydrogen1, nt1 is hydrogen2, and nt2 is oxygen
  Dllist tmp;

  a = ( struct bonding_arg *) arg; //Points to arg
  g = ( global_info *) a->v; //Points to arg
  t = ( thread *) malloc(sizeof( thread));
  t->cond = new_cond();
  t->threadID = a->id; //Sets this thread info to this hyrdrogen's thread info

  pthread_mutex_lock(g->lock); //Lock thread
  
  if((!dll_empty(g->wHydro)) && (!dll_empty(g->wOxygen))) { //Water can be made
    
    /*TAKES THE NEW THREADS, SETS THEIR VALUES, AND SETS ALL OF THE THREAD ID'S*/

    tmp = dll_first(g->wHydro); //Hydrogen on list
    nt1 = tmp->val.v;
    dll_delete_node(tmp); 

    tmp = dll_first(g->wOxygen); //Oxygen on list
    nt2 = tmp->val.v;
    dll_delete_node(tmp);

    nt1->h1 = t->threadID; //Sets the ID's for the first new thread -- New Hydrogen or Second Hydrogen
    nt1->h2 = nt1->threadID;
    nt1->o = nt2->threadID;
    pthread_cond_signal(nt1->cond); 

    nt2->h1 = t->threadID; //Sets the ID's for the second new thread -- Oxygen
    nt2->h2 = nt1->threadID;
    nt2->o = nt2->threadID;
    pthread_cond_signal(nt2->cond);

    t->h1 = t->threadID; //Sets the ID's for the original thread -- Original Hydrogen or First Hydrogen
    t->h2 = nt1->threadID;
    t->o = nt2->threadID;
  }
  else { //Water cannot be made 
    dll_append(g->wHydro, new_jval_v((void *) t)); //Adds the hydrogen to the waiting list
    pthread_cond_wait(t->cond, g->lock);
  }
  pthread_mutex_unlock(g->lock);

  rv = Bond(t->h1, t->h2, t->o); //Bonds the atoms together to make water

  /*FREE UP MEMORY*/
  free(t->cond);
  free(t);

  return rv;
}

void *oxygen(void *arg) { //Almost the same as the hydrogen function with a few changes
  struct bonding_arg *a; //From Lab Notes
  global_info *g; //From Lab Notes
  char *rv; //From Lab Notes
  thread *t, *nt1, *nt2;  //New threads that the oxygen will bond too -- t is oxygen, the other two are hydrogen
  Dllist tmp;         

  a = ( struct bonding_arg *) arg; //Points to arg
  g = ( global_info *) a->v; //Points to arg
  t = ( thread *) malloc(sizeof( thread)); 
  t->cond = new_cond();
  t->threadID = a->id; //Sets this thread info to this hyrdrogen's thread info

  pthread_mutex_lock(g->lock);

  if ((!dll_empty(g->wHydro)) && ((dll_first(g->wHydro)->flink) != dll_nil(g->wHydro))) { //Water can be made
      
    /*TAKES THE NEW THREADS, SETS THEIR VALUES, AND SETS ALL OF THE THREAD ID'S*/

    tmp = dll_first(g->wHydro); //First Hydrogen on the list
    nt1 = tmp->val.v;
    dll_delete_node(tmp); 

    tmp = dll_first(g->wHydro); //Second Hydrogen on the list
    nt2 = tmp->val.v;
    dll_delete_node(tmp);

    nt1->h1 = nt1->threadID; //Sets the ID's for the first new thread -- First Hydrogen
    nt1->h2 = nt2->threadID;
    nt1->o = t->threadID;
    pthread_cond_signal(nt1->cond); 

    nt2->h1 = nt1->threadID; //Sets the ID's for the second new thread -- Second Hyrdrogen
    nt2->h2 = nt2->threadID;
    nt2->o = t->threadID;
    pthread_cond_signal(nt2->cond);

    t->h1 = nt1->threadID; //Sets the ID's for the original new thread -- Oxygen
    t->h2 = nt2->threadID;
    t->o = t->threadID;

  } 
  else { //Water cannot be made
    dll_append(g->wOxygen, new_jval_v((void *) t)); //Adds this oxygen to the waitlist
    pthread_cond_wait(t->cond, g->lock);
  }
  pthread_mutex_unlock(g->lock);
    
  rv = Bond(t->h1, t->h2, t->o);
  
  /*FREE UP MEMORY*/
  free(t->cond);
  free(t);

  return rv;
}
