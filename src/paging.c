
#include "mem.h"
#include "mm.h"
#include "cpu.h"
#include "loader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

int main(int argc, char *argv[])
{

#ifdef MM_PAGING
	if (argc != 3)
	{
		printf("Usage: ./mem [path to configure file] [configuration]\n");
		return 1;
	}

#else
	if (argc != 2)
	{
		printf("Usage: ./mem [path to configure file]\n");
		return 1;
	}

#endif

	char path[100];
	path[0] = '\0';
	strcat(path, "input/proc/");
	strcat(path, argv[1]);

	struct pcb_t *proc = load(path);
	/* Init all MEMPHY include 1 MEMRAM and n of MEMSWP */

	unsigned int i;

#ifdef MM_PAGING
	int sit;
#ifdef MM_FIXED_MEMSZ

	/* We provide here a back compatible with legacy OS simulatiom config file
	 * In which, it has no addition config line for Mema, keep only one line
	 * for legacy info
	 *  [time slice] [N = Number of CPU] [M = Number of Processes to be run]
	 */
	memramsz = 0x100000;
	memswpsz[0] = 0x1000000;
	for (sit = 1; sit < PAGING_MAX_MMSWP; sit++)
		memswpsz[sit] = 0;
#else
	/* Read input config of memory size: MEMRAM and upto 4 MEMSWP (mem swap)
	 * Format: (size=0 result non-used memswap, must have RAM and at least 1 SWAP)
	 *        MEM_RAM_SZ MEM_SWP0_SZ MEM_SWP1_SZ MEM_SWP2_SZ MEM_SWP3_SZ
	 */
	char configPath[100];
	configPath[0] = '\0';
	strcat(configPath, "input/mem_size/");
	strcat(configPath, argv[2]);
	FILE *file = fopen(configPath, "r");
	printf("configPath: %s\n", configPath);
	fscanf(file, "%d\n", &memramsz);
	for (sit = 0; sit < PAGING_MAX_MMSWP; sit++)
		fscanf(file, "%d", &(memswpsz[sit]));

	fscanf(file, "\n"); /* Final character */
						// printf("memramsz: %d\n", memramsz);

#endif

	int rdmflag = 1; /* By default memphy is RANDOM ACCESS MEMORY */
	struct memphy_struct mram;
	struct memphy_struct mswp[PAGING_MAX_MMSWP];

	/* Create MEM RAM */
	init_memphy(&mram, memramsz, rdmflag);

	/* Create all MEM SWAP */
	for (sit = 0; sit < PAGING_MAX_MMSWP; sit++)
		init_memphy(&mswp[sit], memswpsz[sit], rdmflag);

	/* In Paging mode, it needs passing the system mem to each PCB through loader*/
	struct mmpaging_ld_args *mm_ld_args = malloc(sizeof(struct mmpaging_ld_args));
	// printf("Init Paging mode\n");

	mm_ld_args->mram = (struct memphy_struct *)&mram;
	mm_ld_args->mswp = (struct memphy_struct **)&mswp;
	mm_ld_args->active_mswp = (struct memphy_struct *)&mswp[0];
	proc->mm = malloc(sizeof(struct mm_struct));
	init_mm(proc->mm, proc);
	proc->mram = mm_ld_args->mram;
	proc->mswp = mm_ld_args->mswp;
	proc->active_mswp = mm_ld_args->active_mswp;
#endif

	for (i = 0; i < proc->code->size; i++)
	{
		run(proc);
	}
	dump();
	return 0;
}
