#include <stdio.h>
#include <stdlib.h>
#include "locks.h"
#define _GNU_SOURCE

/*Trylock note: should tryLocks initially just examine lock, then call lock? */

void mutexLock(lockargs *args){
	pthread_mutex_lock(args->lockpointer);
}


void mutexUnlock(lockargs *args){
	pthread_mutex_unlock(args->lockpointer);
}

/* TAS lock */
void TASlock(lockargs *args){
	int * lockpointer = args->lockpointer; 
	while (__sync_fetch_and_or(lockpointer, 1)){} //"getAndSet(true)"
}

int tryTASlock(lockargs *args){
	int * lockpointer = args->lockpointer; 
	if(__sync_fetch_and_or(lockpointer, 1) == 0) 
		return 1; //acquired
	else
		return 0; //failed	
}

void TASunlock(lockargs *args){
	//TASstate = 0;
	int * lockpointer = args->lockpointer; 
	__sync_and_and_fetch(lockpointer, 0); // "set(false)"
}

/*Exponential Backoff Lock */

void Backlock(lockargs *args){
	int * lockpointer = args->lockpointer; 

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
	int * lockpointer = args->lockpointer; 

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
	EBOlock = 0; 
}


/*Anderson's Array Lock */

void Alock(lockargs *args){
	int *aTail = args->aTail; 	//This is a pointer.
	int *aFlag = args->aFlag;  	
    int size = args->size;
    int slot = ((__sync_fetch_and_add(aTail,8))) % size;
	(args->mySlot) = slot; 
	while (!aFlag[slot]) {;}
 }

 int tryAlock(lockargs *args){
	int *aTail = args->aTail; 	//This is a pointer.
	int *aFlag = args->aFlag;  	
    int size = args->size;
    int slot = ((__sync_fetch_and_add(aTail,8))) % size;
	(args->mySlot) = slot; 
	if(!aFlag[slot])  
		return 0;
	else 
		return 1;

 }

void Aunlock(lockargs *args){
	int *aFlag = args->aFlag;
	int size = args->size;
	int slot = (args->mySlot); //Must doublecheck if this is being remembered
	aFlag[slot] = 0;
	aFlag[((slot+8) % size)] = 1;
}  


/*CLH lock */

void qlock(lockargs *args){
	qnode *tail = args->qtail;
	qnode **mynode = &(args->mynode);

    (*mynode)->locked = 1;
	
	volatile qnode *pred = __sync_lock_test_and_set(&tail,*mynode);
 	
	(*mynode)->mypred = pred;

   	while (pred->locked){;} 

}

int tryqlock(lockargs *args){
	qnode *tail = args->qtail;
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
	
	qnode **mynode = &(args->mynode);
    
    qnode *pred = (*mynode)->mypred;
	//printf("finishing qunlock. tail has lock %d and id %d\n",tail->locked,tail->id); 
	(*mynode)->locked = 0;
	//printf("finishing qunlock. tail has lock %d and id %d\n",tail->locked,tail->id); 
	*mynode = pred; 
	//printf("finishing qunlock. tail has lock %d and id %d\n",tail->locked,tail->id); 
}

