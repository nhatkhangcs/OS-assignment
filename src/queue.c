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

        /* TODO: put a new process to queue [q] */
        if (q->size < MAX_QUEUE_SIZE)
        {
                int t = q->size;
                q->proc[t] = proc;
                q->size++;
        }
}

#ifdef MLQ_SCHED

struct pcb_t *dequeue(struct queue_t *q)
{
        /* TODO: return a pcb whose prioprity is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (q->size != 0)
        {
                struct pcb_t *t_proc = NULL;
                int highest_priority = 0;
                int i;
                int max_idx = 0;
                for (i = 0; i < q->size; i++)
                {
                        if (q->proc[i]->priority > highest_priority)
                        {
                                t_proc = q->proc[i];
                                highest_priority = q->proc[i]->priority;
                                max_idx = i;
                        }
                }

                for (i = max_idx; i < q->size - 1; i++)
                {
                        q->proc[i] = q->proc[i + 1];
                }
                q->size--;
                return t_proc;
        }
        return NULL;
}

#else

struct pcb_t *dequeue(struct queue_t *q) {
    /* If the queue is empty, return NULL. */
    if (q->size == 0) {
        return NULL;
    }
    //printf("%s\n", "Here");
    /* Get the head element from the front of the queue. */
    struct pcb_t *head = q->proc[0];
    /* Shift all the elements to the left by one position. */
    for (int i = 0; i < q->size - 1; i++) {
        q->proc[i] = q->proc[i+1];
    }
    /* Decrement the size of the queue. */
    q->size--;
    /* Return the head element. */
    return head;
}

#endif


