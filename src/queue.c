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
        // printf("dequeue of MLQ used\n");
        /* TODO: return a pcb whose priority is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (q->size != 0)
        {
                struct pcb_t *t_proc = NULL;
                int highest_priority = MAX_PRIO;
                int i;
                int max_idx = 0;
                for (i = 0; i < q->size; i++)
                {
                        //printf("q->proc[i]->prio: %d\n", q->proc[i]->prio);
                        if (q->proc[i]->prio < highest_priority)
                        {
                                t_proc = q->proc[i];
                                highest_priority = q->proc[i]->prio;
                                //printf("highest_priority: %d\n", highest_priority);
                                max_idx = i;
                        }
                }

                //printf("highest_priority: %d\n", highest_priority);

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

struct pcb_t *dequeue(struct queue_t *q)
{
        // printf("dequeue of non MLQ used\n");
        /* TODO: return a pcb whose priority is the highest
         * in the queue [q] and remember to remove it from q
         * */
        if (q->size != 0)
        {
                struct pcb_t *t_proc = NULL;
                int highest_priority = MAX_PRIO;
                int i;
                int max_idx = 0;
                for (i = 0; i < q->size; i++)
                {
                        if (q->proc[i]->priority < highest_priority)
                        {
                                t_proc = q->proc[i];
                                highest_priority = q->proc[i]->priority;
                                //printf("highest_priority: %d\n", highest_priority);
                                max_idx = i;
                        }
                }

                //printf("highest_priority: %d\n", highest_priority);

                for (i = max_idx; i < q->size - 1; i++)
                {
                        q->proc[i] = q->proc[i + 1];
                }
                q->size--;
                return t_proc;
        }
        return NULL;
}

#endif


