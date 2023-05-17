#include "queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>

static struct queue_t mlq_ready_queue[MAX_PRIO];

void init_scheduler(void)
{
    int i;
    pthread_mutex_init(&mlq_ready_queue->queue_lock, NULL);

    for (i = 0; i < MAX_PRIO; i++)
    {
        mlq_ready_queue[i].size = 0;
        mlq_ready_queue[i].slot = MAX_PRIO - i;
    }
}

struct pcb_t *get_mlq_proc(void)
{
    pthread_mutex_lock(&mlq_ready_queue->queue_lock);
    struct pcb_t *proc = NULL;
    /*TODO: get a process from PRIORITY [ready_queue].
     * Remember to use lock to protect the queue.
     * */
    unsigned long prio;
    int check = 0;
    for (prio = 0; prio < MAX_PRIO; prio++)
        if (!empty(&mlq_ready_queue[prio]))
        {
            if (mlq_ready_queue[prio].slot > 0)
            {
                check = 1;
                proc = dequeue(&mlq_ready_queue[prio]);
                mlq_ready_queue[prio].slot -= 1;
                break;
            }
        }
    if (check == 0)
    {
        int i = 0;
        for (i = 0; i < MAX_PRIO; i++)
        {
            mlq_ready_queue[i].slot = MAX_PRIO - i;
        }
        for (prio = 0; prio < MAX_PRIO; prio++)
            if (!empty(&mlq_ready_queue[prio]))
            {
                if (mlq_ready_queue[prio].slot > 0)
                {
                    proc = dequeue(&mlq_ready_queue[prio]);
                    mlq_ready_queue[prio].slot -= 1;
                    break;
                }
            }
    }
    pthread_mutex_unlock(&mlq_ready_queue->queue_lock);
    return proc;
}

#ifdef MLQ_SCHED
/*
 *  Stateful design for routine calling
 *  based on the priority and our MLQ policy
 *  We implement stateful here using transition technique
 *  State representation   prio = 0 .. MAX_PRIO, curr_slot = 0..(MAX_PRIO - prio)
 */
void put_mlq_proc(struct pcb_t *proc)
{
    pthread_mutex_lock(&mlq_ready_queue->queue_lock);
    enqueue(&mlq_ready_queue[proc->prio], proc);
    pthread_mutex_unlock(&mlq_ready_queue->queue_lock);
}

#else

void put_mlq_proc(struct pcb_t *proc)
{
    pthread_mutex_lock(&mlq_ready_queue->queue_lock);
    enqueue(&mlq_ready_queue[proc->priority], proc);
    pthread_mutex_unlock(&mlq_ready_queue->queue_lock);
}

#endif