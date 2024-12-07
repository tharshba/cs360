/*
 * Taegun Harshbarger
 * COSC 360 Lab 9
 * Sources: Plank's Notes
 */

#include <stdio.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include "fields.h"
#include "jrb.h"
#include "dllist.h"
#include "jval.h"
#include "sockettome.h"

JRB rooms; //Global JRB for the rooms

typedef struct{
	char *name;
	pthread_mutex_t *lock;
	pthread_cond_t *output_cond;
	Dllist chats;
	Dllist users;
} chatRooms;

typedef struct{
	char *name;
	int fd;
	chatRooms *cRooms; 
	FILE *fin, *fout;
} userThreads;

void sigHandler(int s){
	signal(SIGPIPE, sigHandler);
}

void *chatHandler(void *s);
void *roomPrint(void *s);

int main(int argc, char **argv) {
	pthread_t chatThread;
	pthread_t roomThreads[argc-2];	
	userThreads *user;
	chatRooms *roomThread;

	rooms = make_jrb(); //Make tree

	int j=0;
	for(int i = argc-1; i > 1; i--){ //Create the rooms and all of the stuff inside them
		roomThread = (chatRooms *) malloc(sizeof(chatRooms));
		roomThread->name = strdup(argv[i]);
		strcat(roomThread->name, "\0");

		roomThread->users = new_dllist();
		roomThread->chats = new_dllist();

		jrb_insert_str(rooms, strdup(roomThread->name), new_jval_v(roomThread));

		roomThread->lock = (pthread_mutex_t *) malloc(sizeof(pthread_mutex_t));
		pthread_mutex_init(roomThread->lock, NULL);
		roomThread->output_cond = (pthread_cond_t *) malloc(sizeof(pthread_cond_t));
		pthread_cond_init(roomThread->output_cond, NULL);
		pthread_create(&roomThreads[j], NULL, roomPrint, roomThread);
		j++;
	}

	signal(SIGPIPE, sigHandler); //Handles the signal call
	
	int socket = serve_socket(atoi(argv[1]));

	while(1) { //Runs until a connection is accepted
		int fd;
		fd = accept_connection(socket);
		user = (userThreads *) malloc(sizeof(userThreads)); //Makes the user
		user->fd = fd;
		pthread_create(&chatThread, NULL, chatHandler, user); //Runs the rest of the program
	}
	return 0;
}

//Prints the chat
void *roomPrint(void *s) {
	Dllist list;		
	chatRooms *roomThread;		
	userThreads *user; 

	roomThread = (chatRooms *) s;

	pthread_mutex_lock(roomThread->lock);

	while(1){ //Goes through and flushes the chats
		while(!dll_empty(roomThread->chats)){
			dll_traverse(list, roomThread->users){
				user = (userThreads *) jval_v(list->val);
				fputs(roomThread->chats->flink->val.s, user->fout);
				fflush(user->fout);
			}
			dll_delete_node(roomThread->chats->flink);
		}
		pthread_cond_wait(roomThread->output_cond, roomThread->lock);
	}
	return NULL;
}

void *chatHandler(void *s) { //Handles the chat rooms and the users in it
	JRB tmpJRB;
	Dllist userlist;
	userThreads *threads;
	userThreads *tmpThreads;
	chatRooms *roomThread;
	char roomPrint[200];

	//Takes the input and sets up the user to use the server
	threads = (userThreads *) s;
	threads->fin = fdopen(threads->fd, "r");
	threads->fout = fdopen(threads->fd, "w");
	fputs("Chat Rooms:\n\n", threads->fout);
	
	jrb_traverse(tmpJRB, rooms) { //Gets the room names to be printed to the user
		roomThread = (chatRooms *) tmpJRB->val.v;
		pthread_mutex_lock(roomThread->lock);		
		strcpy(roomPrint, roomThread->name);
		strcat(roomPrint, ":");

		if(!dll_empty(roomThread->users)) {
			dll_traverse(userlist, roomThread->users){
				tmpThreads = (userThreads *) jval_v(userlist->val);
				strcat(roomPrint, " ");
				strcat(roomPrint, tmpThreads->name);
			}
		}
		strcat(roomPrint, "\n");
		fputs(roomPrint, threads->fout); //Prints the room names to the user
		pthread_mutex_unlock(roomThread->lock);		
	}
	
	char username[50]; 
	fputs("\nEnter your chat name (no spaces):\n", threads->fout); //Gets the username
	fflush(threads->fout);
	if(fgets(username, 50, threads->fin) == NULL) {
		fclose(threads->fin);
		fclose(threads->fout);
		return NULL;
	}

	if(strlen(username) > 0 && username[strlen(username) - 1] == '\n') {  //Fixes the end of the username to be null char
		username[strlen(username) - 1] = '\0';
	}
	threads->name = strdup(username);

	fputs("Enter chat room:\n", threads->fout);
	fflush(threads->fout);

	char room[50];
	if(fgets(room, 50, threads->fin) == NULL) {
		fclose(threads->fin);
		fclose(threads->fout);
		free(threads->name);
		return NULL;
	}
	if(strlen(room) > 0 && room[strlen(room) - 1] == '\n') { //Fixes the end of the roomname to be null char
		room[strlen(room) - 1] = '\0';
	}

	tmpJRB = jrb_find_str(rooms, room); //Finds the room
		roomThread = (chatRooms *) tmpJRB->val.v;
		threads->cRooms = roomThread;

	//Sends a message that a new user has joined and adds it to the list
	pthread_mutex_lock(threads->cRooms->lock);
	strcpy(username, threads->name);
	strcat(username, " has joined\n");
	dll_append(threads->cRooms->chats, new_jval_s(strdup(username)));
	dll_append(threads->cRooms->users, new_jval_v(threads));
	pthread_cond_signal(threads->cRooms->output_cond);	
	pthread_mutex_unlock(threads->cRooms->lock);

	char userChatter[500];
	while(fgets(userChatter, 500, threads->fin) != NULL) {
		char usernameAndChatter[550];
		strcpy(usernameAndChatter, threads->name);
		strcat(usernameAndChatter, ": ");
		strcat(usernameAndChatter, userChatter);
		pthread_mutex_lock(threads->cRooms->lock);
		dll_append(threads->cRooms->chats, new_jval_s(strdup(usernameAndChatter)));
		pthread_cond_signal(threads->cRooms->output_cond);
		pthread_mutex_unlock(threads->cRooms->lock);
	}

	pthread_mutex_lock(threads->cRooms->lock); //User leaves the room
	strcpy(username, threads->name);
	strcat(username, " has left\n");
	dll_append(threads->cRooms->chats, new_jval_s(strdup(username)));
	Dllist tmp;
	dll_traverse(userlist, threads->cRooms->users) {
		if(jval_v(userlist->val) == threads){
			tmp = userlist;
		}
	}
	dll_delete_node(tmp);
	pthread_cond_signal(threads->cRooms->output_cond);
	pthread_mutex_unlock(threads->cRooms->lock);

	fclose(threads->fin);
	fclose(threads->fout);
	free(threads->name);

	return NULL;
}