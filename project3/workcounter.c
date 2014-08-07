#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Utils/stopwatch.h"
#include "locks.h"
#include <unistd.h>


pthread_mutex_t lock;

volatile int TASlockt = 0;
volatile int EBOlock = 0;
volatile int aTail = 0;
volatile int *aFlag;
volatile qnode *tail;

volatile int count = 0;

typedef struct targs{
	int lockt;
	int count; //thread count goal
	int asize;  
	int nthreads;
	int tid;
}targs; 

double serialcounter(int tcount){
	StopWatch_t tw;
	int privcount = 0;
	
	startTimer(&tw);
	
	while (privcount < tcount){
		privcount++;
		count++;
	}
	stopTimer(&tw);
	
	return getElapsedTime(&tw);
}

void printlockarray(int size){
	int i;
	for(i=0;i<size;i++)
		printf("%d ",aFlag[i]);
		
	printf("\n");

}


void *parallelcounter(void *args){
	targs *arg = (targs *) args; 
	int lockt = arg->lockt;
	int mycount = arg->count;
	int size;

	int privcount = 0; //thread's private counter
	//qnode *mypred;
	//qnode *mynode;
	
	void (*lockfunc) (lockargs *);
	void (*unlockfunc) (lockargs *);
	lockargs *lockarg = (lockargs *)malloc(sizeof(lockargs));


	switch (lockt){
		case 0:
			lockfunc = mutexLock;
			unlockfunc = mutexUnlock;
			lockarg->lockpointer = &lock; 
			//not too sure about the mutex one. maybe a GOTO?
			//other stuff
			break;
		case 1:
			lockfunc = TASlock;
			unlockfunc = TASunlock;
			lockarg->lockpointer = &TASlockt ;
			//other argument assn.
			break;
		case 2:
			lockfunc = Backlock;
			unlockfunc = Backunlock;
			lockarg->lockpointer = &EBOlock;
			//other args;
			break;
		case 3: 
			lockfunc = Alock;
			unlockfunc = Aunlock;
			lockarg->size = (arg->nthreads * 8); //8??
			lockarg->aTail = &aTail;
			lockarg->aFlag = aFlag; 
			break;
		case 4:
			lockfunc = qlock;
			unlockfunc = qunlock;
			lockarg->mynode = (qnode *)malloc(sizeof(qnode));
			lockarg->mynode->id = arg->tid;
			lockarg->mynode->mypred = NULL;
			break;
		default:
			printf("Not a locktype\n");
			pthread_exit(NULL);
	}

	StopWatch_t tw;
		
	startTimer(&tw);
	stopTimer(&tw);
	while(privcount < mycount){
		lockfunc(lockarg);
		count++;
		unlockfunc(lockarg);
		privcount++;
		stopTimer(&tw);
	}
	

	printf("Thread %d counted %d times\n",arg->tid, privcount);
	if (lockt == 4)
		free(lockarg->mynode);
	free(args);
	pthread_exit(NULL);

	/* TO DO: REPLACE SWITCH WITH FUNCTION POINTER */
	/*
	switch(lockt){
		case 0: //mutex
			startTimer(&tw);
			stopTimer(&tw);
			while (privcount < mycount){
				pthread_mutex_lock(&lock);
				count++;
				pthread_mutex_unlock(&lock);
				privcount++;
				stopTimer(&tw); //Why is this here?
			}
			printf("Thread %d counted %d times\n",arg->tid, privcount);
			pthread_exit(NULL);
			break;
		
		case 1: //TASlock
			while (privcount < mycount){
				TASlock();
				count++;
				TASunlock();
				privcount++;
			
			}
			printf("Thread %d counted %d times\n",arg->tid, privcount);
			pthread_exit(NULL);
			break;
		case 2: //backoff 
			while (privcount < mycount){
				Backlock();
				count++;
				Backunlock();
				privcount++;
			}
			printf("Thread %d counted %d times\n",arg->tid, privcount);
			pthread_exit(NULL);
			break;

		case 3: //andersons array lock --broken for WORKCOUNT at the moment 	
			size = arg->asize;
			int nthreads = arg->nthreads;
			int padsize = nthreads*8; 
			int mySlot = 0;
			
			while (privcount < mycount){
				Alock(padsize, &mySlot);
				count++;
				Aunlock(padsize,mySlot);
				privcount++;
			}
			printf("Thread %d counted %d times\n",arg->tid, privcount);		
			pthread_exit(NULL);
			break;
		case 4: 
		      
		   	mynode = (qnode *)malloc(sizeof(qnode));
			mynode->id = arg->tid;
			mynode->mypred = NULL;
		        
			while (privcount < mycount){
			    qlock(&mynode);
				count++;
				qunlock(&mynode);
				privcount++;
			}
			printf("Thread %d counted %d times\n",arg->tid, privcount);
			free(mynode);
			pthread_exit(NULL);
			break;
		
		default:
			printf("not a lock type\n");
			pthread_exit(NULL);
	}
	*/		
}



/* 
mutex = 0;TAS = 1; Backoff = 2; Ander = 3;
*/
int main(int argc, char **argv){
	int tcount; 
	int nthreads;
	int par = 0; 
	//int work = 0; //work or time based count flag
	int c, lockt;

	StopWatch_t tw;
	while ((c = getopt(argc,argv, "c:n:l:p")) != -1){
		switch(c){
			case 'c':
				tcount = atoi(optarg);
				break;
			case 'n':
				nthreads = atoi(optarg);
				break;
			case 'l':
				lockt = atoi(optarg);
				break;
			case 'p':
				par = 1; //running the parallel one
 				break; 
			default:
				nthreads = 1;
				tcount = 128;
				break;
		}
	}


	if (!par){
		double time = serialcounter(tcount);
		printf("serial counted to %d in %f msecs\n", tcount, time);
		return 0;
	}
	
	else{ //parallel 
		int i,rc;
		void *status;
		pthread_t threads[nthreads];
		targs args[nthreads];

		if(lockt == 3){
			aFlag =(int *)malloc(sizeof(int)*nthreads*8);
			aFlag[0] = 1;
		}
		
		if (lockt == 0)
			pthread_mutex_init(&lock, NULL);

		for (i = 0; i < nthreads; i++){

			args[i].lockt = lockt;
			args[i].count = tcount/nthreads; //assume multiple of 4. 
			if(lockt==3){
				args[i].asize = nthreads;
				args[i].nthreads = nthreads;
			}
			args[i].tid = i;
		}
		
		if(lockt == 4){
			tail = (qnode *)malloc(sizeof(qnode));
			tail->locked = 0;
			tail->id = 99999;
			tail->mypred = NULL;
		}
		startTimer(&tw);
		for (i = 0; i < nthreads; i++){
			rc = pthread_create(&threads[i], NULL, parallelcounter, (void *) &args[i]);
			if (rc) {
			  printf("ERROR; return code from pthread_create() is %d\n", rc);
			  exit(-1);
			}
		}
		while(count < tcount){;}
		stopTimer(&tw);
		
		for (i=0; i<nthreads; i++){		  
			rc = pthread_join(threads[i], &status);
		  	if (rc) {
		    printf("ERROR; return code from pthread_join() is %d\n", rc);
		    exit(-1);
		  	}
		}

		if(lockt == 3)
			free((void*)aFlag);
		if(lockt == 4)
		  	free((void *)tail);
		
		printf("Parallel - counted to %d in %f secs\n",count,getElapsedTime(&tw));
		return 0;
	}
}

