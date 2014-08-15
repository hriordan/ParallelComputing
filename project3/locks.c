#include <stdio.h>
#include <stdlib.h>
#include "locks.h"
#define _GNU_SOURCE

/*Trylock note: should tryLocks initially just examine lock, then call lock? */

void mutexLock(lockargs *args){
	pthread_mutex_lock((pthread_mutex_t *)args->lockpointer);
}


void mutexUnlock(lockargs *args){
	pthread_mutex_unlock((pthread_mutex_t *)args->lockpointer);
}

/* TAS lock */
void TASlock(lockargs *args){
	volatile int * lockpointer = args->lockpointer; 
	while (__sync_fetch_and_or(lockpointer, 1)){} //"getAndSet(true)"
}

int tryTASlock(lockargs *args){
	volatile int * lockpointer = args->lockpointer; 
	if(__sync_fetch_and_or(lockpointer, 1) == 0) 
		return 1; //acquired
	else
		return 0; //failed	
}

void TASunlock(lockargs *args){
	//TASstate = 0;
	volatile int * lockpointer = args->lockpointer; 
	__sync_and_and_fetch(lockpointer, 0); // "set(false)"
}

/*Exponential Backoff Lock */

void Backlock(lockargs *args){
	volatile int * lockpointer = args->lockpointer; 

	struct timespec tim;
	tim.tv_sec = 0;

	int mindel = MINDELAY; 
	int maxdel = MAXDELAY; 
	int limit = mindel; 
	unsigned int sp;
	srand(time(NULL));
	while(1){
		while(*lockpointer){;}
		if(__sync_lock_test_and_set(lockpointer,1) == 0)
		//if(__sync_fetch_and_or(&EBOstate, 1) == 0)
			return;
		else{//back off
			tim.tv_nsec = rand_r(&sp) % limit; 
 			limit = maxdel < (2 * limit) ? maxdel : (2 * limit);
			nanosleep(&tim, NULL);
		}
	}
}

int tryBacklock(lockargs *args){
	volatile int * lockpointer = args->lockpointer; 

	struct timespec tim;
	tim.tv_sec = 0;

	int mindel = MINDELAY; 
	int maxdel = MAXDELAY; 
	int limit = mindel; 
	unsigned int sp;
	srand(time(NULL));
	
	if(__sync_lock_test_and_set(lockpointer,1) == 0)
	//if(__sync_fetch_and_or(&EBOstate, 1) == 0)
		return 1;
	else{//back off
		tim.tv_nsec = rand_r(&sp) % limit; 
		limit = maxdel < (2 * limit) ? maxdel : (2 * limit);
		nanosleep(&tim, NULL);
		return 0;
	}

}

void Backunlock(lockargs *args){
	volatile int * lockpointer = args->lockpointer;
	*lockpointer = 0; //atomic zeroing needed? 
	 
}


/*Anderson's Array Lock */

void Alock(lockargs *args){
	volatile int *aTail = args->aTail; 	//This is a pointer.
	volatile int *aFlag = args->aFlag;  	
    int size = args->size;
    int slot = ((__sync_fetch_and_add(aTail,8))) % size;
	(args->mySlot) = slot; 
	while (!aFlag[slot]) {;}
 }

 int tryAlock(lockargs *args){
	volatile int *aTail = args->aTail; 	//This is a pointer.
	volatile int *aFlag = args->aFlag;  	
    int size = args->size;
    int slot = ((__sync_fetch_and_add(aTail,8))) % size;
	(args->mySlot) = slot; 
	if(!aFlag[slot])  
		return 0;
	else 
		return 1;

 }

void Aunlock(lockargs *args){
	volatile int *aFlag = args->aFlag;
	int size = args->size;
	int slot = (args->mySlot); //Must doublecheck if this is being remembered
	aFlag[slot] = 0;
	aFlag[((slot+8) % size)] = 1;
}  


/*CLH lock */
//STICK INCORRECT.
void qlock(lockargs *args){
	volatile qnode **tail = &(args->qtail);
	qnode **mynode = &(args->mynode);

    (*mynode)->locked = 1;
	//printf("mynode id is %d\n", (*mynode)->id);	
	//printf("tail id before is %d\n", (*tail)->id);
	volatile qnode *pred = __sync_lock_test_and_set(tail,*mynode);
 	//printf("tail id is now %d\n", (*tail)->id);	
	(*mynode)->mypred = pred;
	//printf("pred id is %d\n", pred->id);		
   	while (pred->locked){;} 
	//printf("got here 4\n");	
}

int tryqlock(lockargs *args){
	volatile qnode *tail = args->qtail;
	qnode **mynode = &(args->mynode);

    (*mynode)->locked = 1;
	
	volatile qnode *pred = __sync_lock_test_and_set(&tail,*mynode);
 	
	(*mynode)->mypred = pred;

   	if(pred->locked)
   		return 0;
   	else
   		return 1; 

}

void qunlock(lockargs *args){
	volatile qnode *tail = args->qtail;
	//printf("starting qunlock. tail has lock %d and id %d\n",tail->locked,tail->id); 
	qnode **mynode = &(args->mynode); //TBD: need volatile? gives incompat warning
    qnode *pred = ((*mynode)->mypred);
    //printf("qunlock 1. pred has id %d and mynode has id %d\n",pred->id, (*mynode)->id); 
	(*mynode)->locked = 0;
	(*mynode) = pred;
	//printf("mynode id is %d\n", (*mynode)->id);
	//printf("finishing qunlock. tail has lock %d and id %d\n",tail->locked,tail->id); 
	//printf("qunlock 2. pred has id %d and mynode has id %d\n",pred->id,(*mynode)->id);
	//printf("finishing qunlock. tail has lock %d and id %d\n",tail->locked,tail->id); 
}

