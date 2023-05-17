#ifndef QUEUE_H
#define QUEUE_H

#include "common.h"

// #ifndef MLQ_SCHED
// #define MLQ_SCHED
// #endif

#define MAX_PRIO 140

void init_scheduler(void);

/* Get the next process from ready queue */
struct pcb_t * get_mlq_proc(void);

/* Put a new process to ready queue, or put non-terminated process to ready-queue */
void put_mlq_proc(struct pcb_t * proc);

#endif


