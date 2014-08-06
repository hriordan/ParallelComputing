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
volatile int TASlockt = 0;
volatile int EBOlock = 0;
volatile int aTail = 0;
volatile int *aFlag;
volatile qnode *tail;

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

typedef struct queue {
	volatile int head, tail;
	volatile Packet_t *pqueue[Q_LEN];
} queue;

queue *Queue;

volatile long *subprints;

struct args {
	int numPackets;
	int source; 
	//long &fingerprint; // or make it fucking global
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
		//printf("empty fucking queue\n");
		return NULL;
	}
	volatile Packet_t *x = Queue[source].pqueue[Queue[source].head % Q_LEN];
	Queue[source].head++;
	return x;
}

void *worker(void * targs){

	struct args *args;
	args = (struct args *) targs;
	int source = args->source;
	int packetsProcessed = 0;

	volatile Packet_t *temp;
	while (packetsProcessed < args->numPackets){

		if((temp = deq(source)) == NULL)
			continue; //if nothing in queue, spin your wheels
 		subprints[source] += getFingerprint(temp->iterations, temp->seed);
 	
 		//fingerprint += getFingerprint(temp->iterations, temp->seed);
		packetsProcessed++;
	}
	pthread_exit(NULL);
}



int main(int argc, char * argv[]) {

	if(argc >= DEFAULT_NUMBER_OF_ARGS) {
        const int numPackets = atoi(argv[1]);
		const int numSources = atoi(argv[2]);
		const long mean = atol(argv[3]);
		const int uniformFlag = atoi(argv[4]);
		const short experimentNumber = (short)atoi(argv[5]);
		const int type = atoi(argv[6]);
		const int lockt = atoi(argv[7]);
		const int strat = atoi(argv[8]);

		if(type == 0)
        	serialFirewall(numPackets,numSources,mean,uniformFlag,experimentNumber);
        else if(type == 1)
        	parallelFirewall(numPackets,numSources,mean,uniformFlag,experimentNumber,lockt,strat);
        else
        	serialqueue(numPackets,numSources,mean,uniformFlag,experimentNumber);

	}
    return 0;
}

void serialFirewall (int numPackets,
					 int numSources,
					 long mean,
					 int uniformFlag,
					 short experimentNumber)
{
	 PacketSource_t * packetSource = createPacketSource(mean, numSources, experimentNumber);
	 StopWatch_t watch;
	 fingerprint = 0;

	 if( uniformFlag) {
	     startTimer(&watch);
	      for( int i = 0; i < numSources; i++ ) {
	        for( int j = 0; j < numPackets; j++ ) {
	          volatile Packet_t * tmp = getUniformPacket(packetSource,i);
	          fingerprint += getFingerprint(tmp->iterations, tmp->seed);
	        }
	      }
	      stopTimer(&watch);
	    
}	    else {
	      startTimer(&watch);
	      for( int i = 0; i < numSources; i++ ) {
	        for( int j = 0; j < numPackets; j++ ) {
	          volatile Packet_t * tmp = getExponentialPacket(packetSource,i);
	          fingerprint += getFingerprint(tmp->iterations, tmp->seed);
	        }
	      }
	      stopTimer(&watch);
	    }
	    printf("checksum/fingerpr for serial is %ld\n",fingerprint);
	    printf("time: %f\n",getElapsedTime(&watch));
}


void parallelFirewall(int numPackets,
					 int numSources,
					 long mean,
					 int uniformFlag,
					 short experimentNumber, int lockt, int strat){



	int i, j, rc;

	PacketSource_t * packetSource = createPacketSource(mean, numSources, experimentNumber);
	StopWatch_t watch; 		
	fingerprint = 0; //this should be 'global' to the scope of the threads


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
			argarray[i].numPackets = numPackets;
			argarray[i].source = i;
			argarray[i].queue = Queue;

			rc = pthread_create(&threads[i], NULL, worker, (void *) &argarray[i]);
			if (rc) {
    			printf("ERROR; return code from pthread_create() is %d\n", rc);
    			exit(-1);
    		}
//    		printf("thread %d created\n", i);
		}

		//give dem threads some work 
 		for (j = 0; j < numPackets; j++)
 			for(i = 0; i < numSources; i++){
 				volatile Packet_t * tmp = getUniformPacket(packetSource,i);
 				while((enq(tmp, i)) == NULL) //keep trying until you can slot something?
 					{;}
 				//printf("enqueing to source %d on packet %d\n",i,j);
 			}

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
 		//printf("checksum/fingerpr for parallel is %ld\n",fingerprint);
 		printf("time: %f\n",getElapsedTime(&watch));


	}
	else{
		startTimer(&watch);
		//make dem threads
		for (i = 0; i < numSources; i++){
			argarray[i].numPackets = numPackets;
			argarray[i].source = i;
			argarray[i].queue = Queue;

			rc = pthread_create(&threads[i], NULL, worker, (void *) &argarray[i]);
			if (rc) {
    			printf("ERROR; return code from pthread_create() is %d\n", rc);
    			exit(-1);
    		}
		}
		//give dem threads some work 
 		for (j = 0; j < numPackets; j++)
 			for(i = 0; i < numSources; i++){
 				volatile Packet_t * tmp = getExponentialPacket(packetSource,i);
 				while((enq(tmp, i)) == NULL) //keep trying until you can slot something?
 					;
 			}

 		//put dem threads to bed 
 		void *status;
		for (i=0;i<numSources;i++){
				rc = pthread_join(threads[i], &status);
				if (rc) {
	        		printf("ERROR; return code from pthread_join() is %d\n", rc);
	         		exit(-1);
	         	}
			}
		stopTimer(&watch);
		pthread_attr_destroy(&attr);
 		printf("checksum/fingerpr for parallel is %ld\n",fingerprint);
	    printf("time: %f\n",getElapsedTime(&watch));

	}
	
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

