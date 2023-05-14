
#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

#define MAX_QUEUE_SIZE 140
//int timeslot;

struct queue_t {
	pthread_mutex_t queue_lock;
	struct pcb_t * proc[MAX_QUEUE_SIZE];
	int size;
	int slot;
};

void enqueue(struct queue_t * q, struct pcb_t * proc);

struct pcb_t * dequeue(struct queue_t * q);

int empty(struct queue_t * q);

#endif

