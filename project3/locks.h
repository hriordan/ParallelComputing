#include <stdio.h>
#include <stdlib.h>
#include <time.h>


#define MAXDELAY 1000
#define MINDELAY 1 

#define _BSD_SOURCE


typedef struct qnode{
	int locked; //volatile?
  	int id;
  	struct qnode *mypred;
} qnode;

typedef struct lockargs {
	int size;
	int mySlot;
	qnode **mynode; 
} lockargs;

extern volatile int TASlockt;
extern volatile int EBOlock;
extern volatile int count;
extern volatile int aTail;
extern volatile int *aFlag;
extern volatile qnode *tail;

/*Declarations for Lock Types */
void TASlock(lockargs *args);

void TASunlock(lockargs *args);

void Backlock(lockargs *args);

void Backunlock(lockargs *args);

void Alock(lockargs *args);

void Aunlock(lockargs *args);

void qlock(lockargs *args);
	
void qunlock(lockargs *args);
