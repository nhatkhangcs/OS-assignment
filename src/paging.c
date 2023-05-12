
#include "mem.h"
#include "mm.h"
#include "cpu.h"
#include "loader.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef MM_PAGING
static int memramsz;
static int memswpsz[PAGING_MAX_MMSWP];

struct mmpaging_ld_args
{
	/* A dispatched argument struct to compact many-fields passing to loader */
	struct memphy_struct *mram;
	struct memphy_struct **mswp;
	struct memphy_struct *active_mswp;
	struct timer_id_t *timer_id;
};
#endif

int main() {
	//struct pcb_t * ld = load("input/proc/p0s");
	struct pcb_t * proc = load("input/proc/p0s");
	//check proc info
	printf("pid: %d\n", proc->pid);
	printf("priority: %d\n", proc->priority);
	printf("code size: %d\n", proc->code->size);
	/* Init all MEMPHY include 1 MEMRAM and n of MEMSWP */
	int rdmflag = 1; /* By default memphy is RANDOM ACCESS MEMORY */

	struct memphy_struct mram;
	struct memphy_struct mswp[PAGING_MAX_MMSWP];
	
	/* Create MEM RAM */
	init_memphy(&mram, memramsz, rdmflag);

	/* Create all MEM SWAP */
	int sit;
	for (sit = 0; sit < PAGING_MAX_MMSWP; sit++)
		init_memphy(&mswp[sit], memswpsz[sit], rdmflag);

	/* In Paging mode, it needs passing the system mem to each PCB through loader*/
	struct mmpaging_ld_args *mm_ld_args = malloc(sizeof(struct mmpaging_ld_args));
	//printf("Init Paging mode\n");

	mm_ld_args->mram = (struct memphy_struct *)&mram;
	mm_ld_args->mswp = (struct memphy_struct **)&mswp;
	mm_ld_args->active_mswp = (struct memphy_struct *)&mswp[0];
	unsigned int i;
#ifdef MM_PAGING
		proc->mm = malloc(sizeof(struct mm_struct));
		init_mm(proc->mm, proc);
		proc->mram = mm_ld_args->mram;
		proc->mswp = mm_ld_args->mswp;
		proc->active_mswp = mm_ld_args->active_mswp;
#endif
	for (i = 0; i < proc->code->size; i++) {
		run(proc);
		//run(ld);
	}
	dump();
	return 0;
}

