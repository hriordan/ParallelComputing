#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "Utils/generators.h"
#include "Utils/stopwatch.h"
#include "Utils/fingerprint.h"
#include "Utils/packetsource.h"

#define DEFAULT_NUMBER_OF_ARGS 6
#define Q_LEN 8

/*okay. so i think my basic idea shold be to have an array of each lock*/
pthread_mutex_t *lock;
volatile int *TASlockt;
volatile int *EBOlock;
volatile int *aTail;
volatile int **aFlag;
volatile qnode **tail;

void allocateLocks(lock_t locktype, int numSources){
	int i;
	if (locktype == MUTEX){
		lock = (pthread_mutex_t *)malloc(sizeof(pthread_mutex_t) * numSources);
		for(i = 0; i < numSources; i++)
			pthread_mutex_init(&lock[i], NULL); //init all locks
	}
	else if(locktype == TAS)
		TASlockt = (int *)malloc(sizeof(int) * numSources);
	else if(locktype == BACKOFF)
		EBOlock = (int *)malloc(sizeof(int) * nuSources);
	else if(locktype == ALOCK){
		aFlag =(int **)malloc(sizeof(int*)*numSources);
		for (i = 0; i < numSources; i++)
			aFlag[i] = (int *)malloc(sizeof(int)*numSources*4);
			aFlag[i][0] = 1;
	}
	/*Note: a little unsure atm how much gets malloced. definitely the tail*/
	else if (locktype == QLOCK){
		tail = (qnode**)malloc(sizeof(qnode *) * numSources);
		for(i = 0; i < numSources; i++){
			tail = (qnode *)malloc(sizeof(qnode));
			tail->locked = 0;
			tail->id = 99999;
			tail->mypred = NULL;
		}
	}
}

void freeLocks(lock_t locktype, int numSources){
	;
}

typedef enum {LOCKFREE, HOMEQUEUE, RANDOMQUEUE,	LASTQUEUE} strategy_t; 

typedef enum {MUTEX, TAS, BACKOFF, ALOCK, QLOCK} lock_t; 

void serialFirewall(const int,
					const int,
					const long,
					const int,
					const short);

void parallelFirewall(const int,
					const int,
					const long,
					const int,
					const short, const int, const int);
void serialqueue(const int,
					const int,
					const long,
					const int,
					const short);

long fingerprint = 0;

queue *Queue;

volatile long *subprints;

typedef struct queue {
	volatile int head, tail;
	volatile Packet_t *pqueue[Q_LEN];

} queue;

struct args {
	int numPackets;
	float numMilliseconds;
	int source; 
	strategy_t qstrategy; 
	lock_t locktype; 
	queue *queue; 
};

void * enq(volatile Packet_t *packet, int source){
	if ((Queue[source].tail - Queue[source].head) == Q_LEN){
		return NULL;
	}
	Queue[source].pqueue[Queue[source].tail % Q_LEN] = packet;
	Queue[source].tail++;
	return 1;
}

volatile Packet_t *deq(int source){
	if((Queue[source].tail - Queue[source].head) == 0){
		return NULL;
	}
	volatile Packet_t *x = Queue[source].pqueue[Queue[source].head % Q_LEN];
	Queue[source].head++;
	return x;
}


/*Performs work on Queues by dequeuing things according to a set strategy */
void *worker(void * targs){

	struct args *wargs = (struct args *) targs;
	
	void (*lockfunc) (lockargs*);
	void (*unlockfunc) (lockargs*);
	lockargs *lockarg = (lockargs *)malloc(sizeof(lockargs));; 
	int packetsProcessed = 0;
	volatile Packet_t *temp;

	struct args *args;
	args = (struct args *) targs;
	
	switch(args->locktype){
		case MUTEX:
			lockfunc = mutexLock;
			unlockfunc = mutexUnlock;
			
			//not too sure about the mutex one. maybe a GOTO?
			//other stuff
			break;
		case 1:
			lockfunc = TASlock;
			unlockfunc = TASunlock;
			//lockarg->lockpointer = &TASlockt ;
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
			/*
			lockarg->size = (arg->nthreads * 8); //8??
			lockarg->aTail = &aTail;
			lockarg->aFlag = aFlag; 
			*/
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

	

	switch(args->qstrategy){
		case LOCKFREE:
			startTimer(&watch);

			while (getElapsedTime(&watch) <= wargs->numMilliseconds){

				source = wargs->source;
				lockargs->lockpointer = _assignLockPointer(source, args->locktype);	 
				//lockfunc(/*pointer*/);
				if((temp = deq(source)) == NULL){
					stopTimer(&watch);
					if (getElapsedTime(&watch) >= wargs->numMilliseconds)
						break;
					continue; //if nothing in queue, spin your wheels
				}
		 		subprints[source] += getFingerprint(temp->iterations, temp->seed);
		 	
		 		//fingerprint += getFingerprint(temp->iterations, temp->seed);
				packetsProcessed++;
				stopTimer(&watch);
			}
			break;
		case HOMEQUEUE:
			;
			break;
		case RANDOMQUEUE:
			;
			break;
		case LASTQUEUE:
			;
			break;
		default:
			;
			break;

	}
	pthread_exit(NULL);
	//int source = args->source;


	/*
	
	while (packetsProcessed < args->numPackets){

		if((temp = deq(source)) == NULL)
			continue; //if nothing in queue, spin your wheels
 		subprints[source] += getFingerprint(temp->iterations, temp->seed);
 	
 		//fingerprint += getFingerprint(temp->iterations, temp->seed);
		packetsProcessed++;
	}
	pthread_exit(NULL);
	*/
}




int main(int argc, char * argv[]) {

	if(argc >= DEFAULT_NUMBER_OF_ARGS) {
        const float numMilliseconds = atoi(argv[1]);
		const int numSources = atoi(argv[2]);
		const long mean = atol(argv[3]);
		const int uniformFlag = atoi(argv[4]);
		const short experimentNumber = (short)atoi(argv[5]);
		const int type = atoi(argv[6]);
		const int lockt = atoi(argv[7]);
		const int strat = atoi(argv[8]);

		if(type == 0)
        	serialPacket(numMilliseconds,numSources,mean,uniformFlag,experimentNumber);
        else if(type == 1)
        	parallelPacket(numMilliseconds,numSources,mean,uniformFlag,experimentNumber,lockt,strat);


	}
    return 0;
}

void serialPacket (float numMilliseconds,
					 int numSources,
					 long mean,
					 int uniformFlag,
					 short experimentNumber)
{
	PacketSource_t * packetSource = createPacketSource(mean, numSources, experimentNumber);
	StopWatch_t watch;
	fingerprint = 0;
	int packetsProcessed = 0;

	if( uniformFlag) {
	   	startTimer(&watch);
	   	while (1){
		    for( int i = 0; i < numSources; i++ ) {
		        volatile Packet_t * tmp = getUniformPacket(packetSource,i);
		        fingerprint += getFingerprint(tmp->iterations, tmp->seed);
		        packetsProcessed++;
		        stopTimer(&watch);
		        if (getElapsedTime(&watch) >= numMilliseconds)
		        	goto done;
		    }
		    if (getElapsedTime(&watch) >= numMilliseconds)
		        break;
		}
		done:
	    stopTimer(&watch);
	}	 
	else {
	    while (1){
		    for( int i = 0; i < numSources; i++ ) {
		        volatile Packet_t * tmp = getExponentialPacket(packetSource,i);
		        fingerprint += getFingerprint(tmp->iterations, tmp->seed);
		        packetsProcessed++;
		        stopTimer(&watch);
		        if (getElapsedTime(&watch) >= numMilliseconds)
		        	goto done;
		    }
		    if (getElapsedTime(&watch) >= numMilliseconds)
		        break;
		}

		done:
	    stopTimer(&watch);
	}
	    printf("checksum/fingerpr for serial is %ld\n",fingerprint);
	    printf("Serial counted %d packets\n",packetsProcessed);
	    printf("time: %f\n",getElapsedTime(&watch));
}


void parallelFirewall(float numMilliseconds,
					 int numSources,
					 long mean,
					 int uniformFlag,
					 short experimentNumber, int lockt, int strat){

	int i, j, rc;

	PacketSource_t * packetSource = createPacketSource(mean, numSources, experimentNumber);
	StopWatch_t watch; 		
	//fingerprint = 0; //this should be 'global' to the scope of the threads

	allocateLocks(lock_t lock, int numSources);

	Queue = (queue *)malloc(sizeof(queue)*numSources); 
	subprints = (long *)malloc(sizeof(long)*numSources);

	for (i = 0; i < numSources; i++){
		Queue[i].tail = 0;
		Queue[i].head = 0;
	}
	
	pthread_t threads[numSources];
	struct args argarray[numSources]; 
	pthread_attr_t attr; 
	pthread_attr_init (&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);	



	if(uniformFlag){
		startTimer(&watch);

		//make dem threads
		for (i = 0; i < numSources; i++){
			argarray[i].numMilliseconds = numMilliseconds;
			argarray[i].source = i;
			argarray[i].queue = Queue; //pointer to the queue
			argarray[i].qstrategy = strat;
			argarray[i].locktype = lockt;


			rc = pthread_create(&threads[i], NULL, worker, (void *) &argarray[i]);
			if (rc) {
    			printf("ERROR; return code from pthread_create() is %d\n", rc);
    			exit(-1);
    		}
		}

		//give dem threads some work 
 		while(1){
 			for(i = 0; i < numSources; i++){
 				volatile Packet_t * tmp = getUniformPacket(packetSource,i);
 				while((enq(tmp, i)) == NULL) //keep trying until you can slot something?
 					{;}
 				stopTimer(&watch);
 				if(getElapsedTime(&watch) >= numMilliseconds){
 					goto done;
 				}
 			}
 			stopTimer(&watch);
 			if(getElapsedTime(&watch) >= numMilliseconds){
 				goto done;
 		}

 	done: 
 		//put dem threads to bed 
 		void *status;
		for (i=0;i<numSources;i++){
				rc = pthread_join(threads[i], &status);
				if (rc) {
	        		printf("ERROR; return code from pthread_join() is %d\n", rc);
	         		exit(-1);
	         	}
			}

		for (i = 0; i < numSources; i++){
			//printf("queue %d has head %d and tail %d\n",i, Queue[i].head, Queue[i].tail);
			fingerprint += subprints[i];
		}
		stopTimer(&watch);
		pthread_attr_destroy(&attr);
		printf("alternate checksumming parallel is %ld\n", fingerprint);
 		printf("time: %f\n",getElapsedTime(&watch));


	}
	else{
			startTimer(&watch);

		//make dem threads
		for (i = 0; i < numSources; i++){
			argarray[i].numMilliseconds = numMilliseconds;
			argarray[i].source = i;
			argarray[i].queue = Queue; //pointer to the queue
			argarray[i].qstrategy = strat;
			argarray[i].locktype = lockt;


			rc = pthread_create(&threads[i], NULL, worker, (void *) &argarray[i]);
			if (rc) {
    			printf("ERROR; return code from pthread_create() is %d\n", rc);
    			exit(-1);
    		}
		}

		//give dem threads some work 
 		while(1){
 			for(i = 0; i < numSources; i++){
 				volatile Packet_t * tmp = getExponentialPacket(packetSource,i);
 				while((enq(tmp, i)) == NULL) //keep trying until you can slot something?
 					{;}
 				stopTimer(&watch);
 				if(getElapsedTime(&watch) >= numMilliseconds){
 					goto done;
 				}
 			}
 			stopTimer(&watch);
 			if(getElapsedTime(&watch) >= numMilliseconds){
 				goto done;
 		}

 	done: 
 		//put dem threads to bed 
 		void *status;
		for (i=0;i<numSources;i++){
				rc = pthread_join(threads[i], &status);
				if (rc) {
	        		printf("ERROR; return code from pthread_join() is %d\n", rc);
	         		exit(-1);
	         	}
			}

		for (i = 0; i < numSources; i++){
			//printf("queue %d has head %d and tail %d\n",i, Queue[i].head, Queue[i].tail);
			fingerprint += subprints[i];
		}
		stopTimer(&watch);
		pthread_attr_destroy(&attr);
		printf("alternate checksumming parallel is %ld\n", fingerprint);
 		printf("time: %f\n",getElapsedTime(&watch));


	}
	
	freeLocks(lockt, int numSources);
	free (Queue);	
	free ((void *)subprints);

}

void serialqueue(int numPackets,
					 int numSources,
					 long mean,
					 int uniformFlag,
					 short experimentNumber){

	int i, j;

	Queue = (queue *)malloc(sizeof(queue)*numSources); 
	PacketSource_t * packetSource = createPacketSource(mean, numSources, experimentNumber);
	StopWatch_t watch; 		
	fingerprint = 0;

	for (i = 0; i < numSources; i++){
		Queue[i].tail = 0;
		Queue[i].head = 0;
	}
	if(uniformFlag){	
		
		startTimer(&watch);
		
		for (j = 0; j < numPackets; j++)
	 		for(i = 0; i < numSources; i++){
	 			volatile Packet_t * tmp = getUniformPacket(packetSource,i);
	 			enq(tmp, i);
	 			tmp = deq(i);
	 			fingerprint += getFingerprint(tmp->iterations, tmp->seed);	
	 		}
	 	stopTimer(&watch);	
	 	printf("checksum/fingerpr for serial-queue is %ld\n",fingerprint);
	    printf("time: %f\n",getElapsedTime(&watch));
	 }
}

