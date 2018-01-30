/*
 * transfProg.c
 *
 *  Created on: Feb 11, 2017
 *      Author: gowtham
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <pthread.h>
#include <unistd.h>

struct account {
	int id;
	int value;
	int status; // 0 not used; 1 used
};
typedef struct account account;

account *bank;
int num_acc = 0; // total number of account

char **transfer; //storing the transfer details and they are like %d %d %d (acc1, acc2, money to transfer)
int transfer_count = 0;

struct thread {
	int acc1loc;
	int acc2loc;
};

typedef struct thread thread;

thread *thread_details; // to indicate which threads what accounts

int numthread; // no. of thread

pthread_cond_t *cond; // array of conditions so that each thread can wait for the other thread to finish
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER; //used for checking global status

int acc_loc(int accno) {
	int i;
	for(i = 0; i < num_acc; i++) {
		if(accno == bank[i].id)
			return i;
	}
}

void take_forks(int acc1loc, int acc2loc, int threadid) {
	pthread_mutex_lock (&lock);

	while( (bank[acc1loc].status == 1) || (bank[acc1loc].status == 1) ){ //if any one of the account is used the thread wait

		thread_details[threadid].acc1loc = acc1loc; //to indicate other threads are waiting!!
		thread_details[threadid].acc2loc = acc2loc;

		pthread_cond_wait(&cond[threadid], &lock);
	}

	bank[acc1loc].status = 1;
	bank[acc2loc].status = 1;

	pthread_mutex_unlock (&lock);
}

void put_forks(int acc1loc, int acc2loc, int threadid) {
	pthread_mutex_lock (&lock);

	bank[acc1loc].status = 0;
	bank[acc2loc].status = 0;

	thread_details[threadid].acc1loc = -1; // making -1 indicating it doesn't need account
	thread_details[threadid].acc2loc = -1;

	for(int i = 0; i < numthread; i++) { // signalling the dependent threads

		if( (thread_details[i].acc1loc == acc1loc) || (thread_details[i].acc1loc == acc2loc))
			pthread_cond_signal(&cond[i]);
		else if( (thread_details[i].acc2loc == acc1loc) || (thread_details[i].acc2loc == acc2loc))
			pthread_cond_signal(&cond[i]);
	}
	pthread_mutex_unlock (&lock);
}

void handler ( void *ptr )
{

	int threadid;
	threadid = *((int *) ptr);

    for(int i = threadid; i < transfer_count; i = i + numthread) {

		int acc1, acc2, transfer_amount;
		sscanf(transfer[i], "%d %d %d", &acc1, &acc2, &transfer_amount); // transfer from acc1 to acc2

		int acc1loc = acc_loc(acc1);
		int acc2loc = acc_loc(acc2);

		//printf("$$Thread %d, %d to %d taking forks \n",threadid, acc1, acc2);
		take_forks(acc1loc, acc2loc, threadid);
		//printf("---------Thread %d, %d to %d transferring \n",threadid, acc1, acc2);
		bank[acc1loc].value -= transfer_amount;
		bank[acc2loc].value += transfer_amount;

		//sleep(1);
		//printf("!!Thread %d, %d to %d putting_forks \n",threadid, acc1, acc2);
		put_forks(acc1loc, acc2loc, threadid);
		//printf("**Thread %d, %d to %d ending \n",threadid, acc1, acc2);

    }
    pthread_exit(0); /* exit thread */
}

int main(int argc, char** argv) {

		numthread = atoi(argv[2]);
		//printf("numthread = %d\n",numthread);

		thread_details = malloc(sizeof(thread) * numthread);

		pthread_t * pthreadArray;
		pthreadArray = malloc(sizeof(pthread_t) * numthread);

		cond = malloc(sizeof(pthread_cond_t) * numthread);

		for(int i = 0; i < numthread; i++) { // making -1 indicating it doesn't need account
			thread_details[i].acc1loc = -1;
			thread_details[i].acc2loc = -1;
			pthread_cond_init (&cond[i], NULL);
		}

		char * subject = NULL;
	  	size_t len = 0;
	  	ssize_t read;

	  	FILE * fp;
	  	//fp = fopen("ip.txt", "r");
	  	fp = fopen(argv[1], "r");
	  	if (fp == NULL)
	  		exit(EXIT_FAILURE);
	  	//int i = 0;

	  	num_acc = 0;
	  	transfer_count = 0;
		while ((read = getline(&subject, &len, fp)) != EOF) {
			if(isdigit(subject[0])){
				num_acc++;
			}
			else if(subject[0] == 'T'){
				transfer_count++;
			}
		}

		bank = malloc(num_acc * (sizeof(account)) );

		transfer = malloc(sizeof(char*) * transfer_count); // allocating pointers
		for(int i = 0; i < transfer_count; i++) {
			transfer[i] = malloc(40 * sizeof(char)); //each pointer has 40 char length
		}
		rewind(fp);

		int i = 0;
		int j = 0;
	  	while ((read = getline(&subject, &len, fp)) != EOF) {
	  		if(subject[0] == 'T'){
	  			strcpy(transfer[j++],subject+9);

	  		}
	  		else if(isdigit(subject[0])){
	  			int x = 0, y = 0;
	  			sscanf(subject, "%d %d", &x, &y);
	  			bank[i].id = x;
	  			bank[i].value = y;
	  			bank[i].status = 0;
	  			i++;
	  		}
	  	}

	  	fclose(fp);
		if(subject)
			free(subject);

		int * threadid = malloc(sizeof(int) * numthread);

	    for (int j = 0; j < numthread; j++) {
	    	threadid[j] = j;
	    	pthread_create (&pthreadArray[j], NULL, (void *) &handler, (void *) &threadid[j]);
		}

	    for (int j = 0; j < numthread; j++) {
	    	pthread_join(pthreadArray[j], NULL);
		}

	    for (int j = 0; j < num_acc; j++) {
	    	printf("%d %d\n", bank[j].id, bank[j].value);
	    }
	  return 0;
}



