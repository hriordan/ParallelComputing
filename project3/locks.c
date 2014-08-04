#include <stdio.h>
#include <stdlib.h>
#include "locks.h"
#define _GNU_SOURCE




/* TAS lock */
void TASlock(lockargs *args){
	while (__sync_fetch_and_or(&TASlockt, 1)){} //"getAndSet(true)"
}

void TASunlock(lockargs *args){
	//TASstate = 0;
	__sync_and_and_fetch(&TASlockt, 0); // "set(false)"
}

/*Exponential Backoff Lock */

void Backlock(lockargs *args){
	struct timespec tim;
	tim.tv_sec = 0;

	int mindel = MINDELAY; 
	int maxdel = MAXDELAY; 
	int limit = mindel; 
	unsigned int sp;
	srand(time(NULL));
	while(1){
		while(EBOlock){;}
		if(__sync_lock_test_and_set(&EBOlock,1) == 0)
		//if(__sync_fetch_and_or(&EBOstate, 1) == 0)
			return;
		else{//back off
			tim.tv_nsec = rand_r(&sp) % limit; 
 			limit = maxdel < (2 * limit) ? maxdel : (2 * limit);
			nanosleep(&tim, NULL);
		}
	}
}

void Backunlock(lockargs *args){
	EBOlock = 0; 
}


/*Anderson's Array Lock */

void Alock(lockargs *args){ 	
    int size = args->size;
    int slot = ((__sync_fetch_and_add(&aTail,8))) % size;
	(args->mySlot) = slot; 
	while (!aFlag[slot]) {;}
 }

void Aunlock(lockargs *args){
	int slot = (args->mySlot); //Must doublecheck if this is being remembered
	aFlag[slot] = 0;
	aFlag[((slot+8) % size)] = 1;
}  


/*CLH lock */

void qlock(lockargs *args){

	qnode **mynode = &(args->mynode);

    (*mynode)->locked = 1;
	
	volatile qnode *pred = __sync_lock_test_and_set(&tail,*mynode);
 	
	(*mynode)->mypred = pred;

   	while (pred->locked){;} 

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

