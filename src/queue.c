#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

int empty(struct queue_t *q)
{
        if (q == NULL)
                return 1;
        return (q->size == 0);
}

void enqueue(struct queue_t *q, struct pcb_t *proc)
{

        /* TODO: put a new process to queue[q] */
        if (q->size < MAX_QUEUE_SIZE)
        {
                int t = q->size;
                q->proc[t] = proc;
                q->size++;
        }
}

struct pcb_t *dequeue(struct queue_t *q)
{
        /* 
         * TODO: return a pcb whose priority is the highest
         * in the queue [q] and remember to remove it from q
         */
        if (q->size != 0)
        {
                struct pcb_t *t_proc = q->proc[0];
                int i = 0;
                for (i = 0; i < q->size - 1; i++)
                {
                        q->proc[i] = q->proc[i + 1];
                }
                q->size--;
                return t_proc;
        }
        
        return NULL;
}


