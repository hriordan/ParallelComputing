#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Utils/stopwatch.h"
#include "locks.h"
#include <pthread.h>
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
	float secs;
	int asize;  
	int nthreads;
	int tid;
}targs; 

void serialcounter(int time){
	float convTime = ((float)time)/1000.00; //convert secs to msecs  
	printf("convTime is %f\n",convTime);
	StopWatch_t tw;

	startTimer(&tw);
	stopTimer(&tw);
	while (getElapsedTime(&tw) <= convTime){
		//printf("Time: %f\n", getElapsedTime(&tw));
		count++;
		stopTimer(&tw);
	}
}


void *parallelcounter(void *args){
	targs *arg = (targs *) args; 
	int lockt = arg->lockt;
	float t = arg->secs;
	int privcount = 0; //thread's private counter
	

	void (*lockfunc) (lockargs*);
	void (*unlockfunc) (lockargs*);
	lockargs *lockarg = (lockargs *)malloc(sizeof(lockargs));


	switch (lockt){
		case 0:
			lockfunc = mutexLock;
			unlockfunc = mutexUnlock;
			lockarg->lockpointer = &lock; 
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
			lockarg->qtail = tail;
			break;
		default:
			printf("Not a locktype\n");
			pthread_exit(NULL);
	}

	StopWatch_t tw;

	startTimer(&tw);
	stopTimer(&tw);
	while(getElapsedTime(&tw) <= t){
		lockfunc(lockarg);
		count++;
		unlockfunc(lockarg);
		privcount++;
		stopTimer(&tw);
	}

	printf("Thread %d counted %d times\n",arg->tid, privcount);
	//How the fuck do I avoid early freeing pointers while other threads are still using them?
	sleep(1);

	if (lockt == 4)
		free(lockarg->mynode);
	free(lockarg);
	
	pthread_exit((void *)privcount);

	
	
	/*
	switch(lockt){
		case 0: //mutex
			startTimer(&tw);
			stopTimer(&tw);
			while (getElapsedTime(&tw) <= t){
				pthread_mutex_lock(&lock);
				count++;
				pthread_mutex_unlock(&lock);
				privcount++;
				stopTimer(&tw);
			}
			printf("Thread %d counted %d times\n",arg->tid, privcount);
			pthread_exit(NULL);
			break;
		case 1: //TASlock

			startTimer(&tw); //should timer be an arg?
			stopTimer(&tw);
			while (getElapsedTime(&tw) <= t){
				TASlock();
				count++;
				TASunlock();
				privcount++;
				stopTimer(&tw);
			}
			printf("Thread %d counted %d times\n",arg->tid, privcount);
			pthread_exit(NULL);
			break;

		case 2: //backoff 
			startTimer(&tw);
			stopTimer(&tw);
			while (getElapsedTime(&tw) <= t){
				Backlock();
				count++;
				Backunlock();
				privcount++;
				stopTimer(&tw);
			}
			printf("Thread %d counted %d times\n",arg->tid, privcount);
			pthread_exit(NULL);
			break;

		case 3: //andersons array lock		
		    nthreads = arg->nthreads;
			padsize = nthreads*8; 
		
			startTimer(&tw);
			stopTimer(&tw);
			while (getElapsedTime(&tw) <= t){
				Alock(padsize, &mySlot);
				count++;
				Aunlock(padsize,mySlot);
				privcount++;
				stopTimer(&tw);
			}
			printf("Thread %d counted %d times\n",arg->tid, privcount);		
			printf("thread %d exiting\n",arg->tid);
		
			pthread_exit(NULL);
			break;

		case 4:   
		  
		        
			startTimer(&tw);
			stopTimer(&tw);
			while (getElapsedTime(&tw) <= t){
			   	qlock(&mynode);
				count++;
				qunlock(&mynode);
				privcount++;
				stopTimer(&tw);
			}


			printf("Thread %d counted %d times\n",arg->tid, privcount);
			free(mynode);
			printf("thread %d exiting\n",arg->tid);
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

	int msecs = 0; //time to run serial test
	int nthreads;
	int par = 0; 
	//int work = 0; //work or time based count flag
	int c, lockt;

	while ((c = getopt(argc,argv, "t:n:l:p")) != -1){
		switch(c){
			case 't':
				msecs = atoi(optarg);
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
				msecs = 10;
				break;
		}
	}


	if (!par){
		serialcounter(msecs);
		printf("serial counted to %d in %d msecs\n", count, msecs);
		return 0;
	}

	else{ //Running parallel counters 
		int i,rc;
		void *status;
		pthread_t threads[nthreads];
		targs args[nthreads];

		float secs = ((float)msecs)/1000.0;
		printf("parallel convtime is %f\n",secs);
		
		if(lockt == 3){
			aFlag =(int *)malloc(sizeof(int)*nthreads*8); // why *4?
			aFlag[0] = 1;
		}
		
		if (lockt == 0)
			pthread_mutex_init(&lock, NULL);
		
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
		for (i = 0; i < nthreads; i++){

			args[i].lockt = lockt;
			args[i].secs = secs;

			if(lockt==3){
				args[i].asize = nthreads;
				args[i].nthreads = nthreads;
			}

			args[i].tid = i;

			if(lockt == 4){
			  	tail = (qnode *)malloc(sizeof(qnode));
				tail->locked = 0;
				tail->id = 99999;
				tail->mypred = NULL;
			}

			rc = pthread_create(&threads[i], &attr, parallelcounter, (void *) &args[i]);
			if (rc) {
			  printf("ERROR; return code from pthread_create() is %d\n", rc);
			  exit(0);
			}
		}
		pthread_attr_destroy(&attr);
		int sumprivcounts = 0;
		for (i=0; i<nthreads; i++){
			rc = pthread_join(threads[i], &status);
			sumprivcounts += (int) status;
		  	if (rc) {
		    printf("ERROR; return code from pthread_join() is %d\n", rc);
		    exit(0);
		  	}	
			printf("thread %d joined\n",i);
		}
		printf("Parallel - counted to %d in %d msecs\n",count,msecs);
		printf("sum of private thread counts is %d\n",sumprivcounts);
		if(lockt == 3)
				free((void*)aFlag);
		if(lockt == 4)
		  		free((void *)tail);
		
		pthread_exit(NULL);
		return 0;
	}
}

