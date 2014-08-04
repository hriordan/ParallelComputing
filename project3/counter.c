#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "Utils/stopwatch.h"
#include "locks.h"
#include <unistd.h>
//NEEDS DEBUGGING

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
	float convTime = time/1000; //convert msecs to secs 
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
	int size;
	int mySlot;
	int nthreads;
	int padsize;
	int privcount = 0; //thread's private counter
	//qnode *mypred;
	qnode *mynode;

	void *lockfunc (lockargs)
	StopWatch_t tw;


	/*TODO: SWITCH TO FUNCTION POINTER */
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
		    mynode = (qnode *)malloc(sizeof(qnode));
			mynode->id = arg->tid;
			mynode->mypred = NULL;
		        
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

		if(lockt == 3){
			aFlag =(int *)malloc(sizeof(int)*nthreads*4);
			aFlag[0] = 1;
		}
		float secs = msecs/1000;
		
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
			  exit(-1);
			}
		}
		pthread_attr_destroy(&attr);
		for (i=0; i<nthreads; i++){
			rc = pthread_join(threads[i], &status);
		  	if (rc) {
		    printf("ERROR; return code from pthread_join() is %d\n", rc);
		    exit(-1);
		  	}
			printf("thread %d joined\n",i);
		}
		printf("Parallel - counted to %d in %d msecs\n",count,msecs);

		if(lockt == 3)
				free((void*)aFlag);
		if(lockt == 4)
		  		free((void *)tail);
		
		pthread_exit(NULL);
		return 0;
	}
}

