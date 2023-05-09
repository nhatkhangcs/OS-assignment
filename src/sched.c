#include "queue.h"
#include "sched.h"
#include <pthread.h>

#include <stdlib.h>
#include <stdio.h>
static pthread_mutex_t queue_lock;
static struct queue_t mlq_ready_queue[MAX_PRIO];


int queue_empty(void)
{
    unsigned long prio;
    for (prio = 0; prio < MAX_PRIO; prio++)
        if (!empty(&mlq_ready_queue[prio]))
            return -1;
    
    return 1;
}

void init_scheduler(void)
{
    int i;

    for (i = 0; i < MAX_PRIO; i++){
        mlq_ready_queue[i].size = 0;
        mlq_ready_queue[i].slot = MAX_PRIO - i;
    }
}

struct pcb_t *get_mlq_proc(void)
{
    struct pcb_t *proc = NULL;
    /*TODO: get a process from PRIORITY [ready_queue].
     * Remember to use lock to protect the queue.
     * */
    pthread_mutex_lock(&queue_lock);
    unsigned long prio;
    for (prio = 0; prio < MAX_PRIO; prio++)
        if (!empty(&mlq_ready_queue[prio]))
        {
            if (mlq_ready_queue[prio].slot > 0)
            {
                proc = dequeue(&mlq_ready_queue[prio]);
                //mlq_ready_queue[prio].slot -= min(mlq_ready_queue[prio].slot, min(proc->burst_time, timeslot));
                break;
            }

        }
    pthread_mutex_unlock(&queue_lock);
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
    pthread_mutex_lock(&queue_lock);
    enqueue(&mlq_ready_queue[proc->prio], proc);
    pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t *proc)
{
    pthread_mutex_lock(&queue_lock);
    enqueue(&mlq_ready_queue[proc->prio], proc);
    pthread_mutex_unlock(&queue_lock);
}

#else

void put_mlq_proc(struct pcb_t *proc)
{
    pthread_mutex_lock(&queue_lock);
    enqueue(&mlq_ready_queue[proc->priority], proc);
    pthread_mutex_unlock(&queue_lock);
}

void add_mlq_proc(struct pcb_t *proc)
{
    pthread_mutex_lock(&queue_lock);
    enqueue(&mlq_ready_queue[proc->priority], proc);
    pthread_mutex_unlock(&queue_lock);
}

#endif

struct pcb_t *get_proc(void)
{
    return get_mlq_proc();
}

void put_proc(struct pcb_t *proc)
{
    return put_mlq_proc(proc);
}

void add_proc(struct pcb_t *proc)
{
    return add_mlq_proc(proc);
}
