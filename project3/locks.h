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
	int *myslot;
	qnode **mynode; 
}

extern volatile int TASlockt;
extern volatile int EBOlock;
extern volatile int count;
extern volatile int aTail;
extern volatile int *aFlag;
extern volatile qnode *tail;

/*Declarations for Lock Types */
void TASlock();

void TASunlock();

void Backlock();

void Backunlock();

void Alock(int size, int *mySlot);

void Aunlock(int size, int mySlot);

void qlock(qnode **mynode);

void qunlock(qnode **mynode);
