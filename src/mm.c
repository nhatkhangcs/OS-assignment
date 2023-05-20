#include "../include/os-cfg.h"

#ifdef MM_PAGING
/*
 * PAGING based Memory Management
 * Memory management unit mm/mm.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/*
 * init_pte - Initialize PTE entry
 */
int init_pte(uint32_t *pte,
             int pre,    // present
             int fpn,    // FPN
             int drt,    // dirty
             int swp,    // swap
             int swptyp, // swap type
             int swpoff) // swap offset
{
  if (pre != 0)
  {
    if (swp == 0)
    { // Non swap ~ page online
      if (fpn == 0)
        return -1; // Invalid setting

      /* Valid setting with FPN */
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);
    }
    else
    { // page swapped
      SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
      SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);
      CLRBIT(*pte, PAGING_PTE_DIRTY_MASK);

      SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
      SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);
    }
  }

  return 0;
}

/*
 * pte_set_swap - Set PTE entry for swapped page
 * @pte    : target page table entry (PTE)
 * @swptyp : swap type
 * @swpoff : swap offset
 */
int pte_set_swap(uint32_t *pte, int swptyp, int swpoff)
{
  CLRBIT(*pte, PAGING_PTE_PRESENT_MASK);
  SETBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, swptyp, PAGING_PTE_SWPTYP_MASK, PAGING_PTE_SWPTYP_LOBIT);
  SETVAL(*pte, swpoff, PAGING_PTE_SWPOFF_MASK, PAGING_PTE_SWPOFF_LOBIT);

  //printf("pte set to %08x\n", *pte);

  return 0;
}

/*
 * pte_set_fpn - Set PTE entry for on-line page
 * @pte   : target page table entry (PTE)
 * @fpn   : frame page number (FPN)
 */
int pte_set_fpn(uint32_t *pte, int fpn)
{
  SETBIT(*pte, PAGING_PTE_PRESENT_MASK);
  CLRBIT(*pte, PAGING_PTE_SWAPPED_MASK);

  SETVAL(*pte, fpn, PAGING_PTE_FPN_MASK, PAGING_PTE_FPN_LOBIT);

  return 0;
}

/*
 * vmap_page_range - map a range of page at aligned address
 */
int vmap_page_range(struct pcb_t *caller,           // process call
                    int addr,                       // start address which is aligned to pagesz
                    int pgnum,                      // num of mapping page
                    struct framephy_struct *frames, // list of the mapped frames
                    struct vm_rg_struct *ret_rg)    // return mapped region, the real mapped fp
{                                                   // no guarantee all given pages are mapped
  int pgit = 0;
  int pgn = PAGING_PGN(addr);

  ret_rg->rg_end = ret_rg->rg_start = addr; // at least the very first space is usable

  struct framephy_struct *traverse = frames;
  struct mm_struct *mm = caller->mm;

  while (traverse != NULL)
  {
    /* TODO map range of frame to address space
     *      [addr to addr + pgnum*PAGING_PAGESZ
     *      in page table caller->mm->pgd[]
     */
    uint32_t pte = 0;
    pte_set_fpn(&pte, traverse->fpn);
    mm->pgd[pgn + pgit] = pte;
    //printf("page table entry address = %p\n", &mm->pgd[pgn+pgit]);
    pthread_mutex_lock(&caller->mram->fifo_lock);
    enlist_pgn_node(&caller->mram->fifo_fp_list, &mm->pgd[pgn + pgit], traverse->fpn);
    pthread_mutex_unlock(&caller->mram->fifo_lock);

    /* Update return region */
    ret_rg->rg_end += PAGING_PAGESZ;
    pgit++;
    traverse = traverse->fp_next;
  }

  return 0;
}

/*
 * alloc_pages_range - allocate req_pgnum of frame in ram
 * @caller    : caller
 * @req_pgnum : request page num
 * @frm_lst   : frame list
 */
int alloc_pages_range(struct pcb_t *caller, int req_pgnum, struct framephy_struct **frm_lst)
{
  int pgit, fpn;
  struct framephy_struct *newfp_str = NULL;

  //printf("req page %d\n", req_pgnum);
  for (pgit = 0; pgit < req_pgnum; pgit++)
  {
    int freefp_found = MEMPHY_get_freefp(caller->mram, &fpn);
    if (freefp_found < 0) // Not enough frame, must swap
    {
      printf("Alloc pages: not enough free frames in ram\n");
      int swpfpn;
      uint32_t* victim_pte; //pointer to page table entry

      /* Find victim page */
      if (find_victim_page(caller->mram, &victim_pte) < 0) return -1;
      
      int victim_fpn = PAGING_PTE_FPN(*victim_pte);

      /* Get free frame in MEMSWP */
      int free_swp = MEMPHY_get_freefp(caller->active_mswp, &swpfpn);
      if (free_swp < 0) return -1;

      /* Do swap frame from MEMRAM to MEMSWP and vice versa*/
      /* Copy victim frame to swap */
      __swap_cp_page(caller->mram, victim_fpn, caller->active_mswp, swpfpn); // potential param type mismatch

      /* Update page table */
      /* Update the victim page entry to SWAPPED */
      pte_set_swap(victim_pte, 0, swpfpn);
      /* Update the new page entry to FPN */
      //print_pgtbl(caller, 0, -1);
      fpn = victim_fpn;
    }
    // Put the frame number into frm_lst
    newfp_str = malloc(sizeof(struct framephy_struct));
    newfp_str->fpn = fpn;
    newfp_str->owner = caller->mm;
    
    //insert newfp_str at tail of frm_lst
    if(*frm_lst == NULL){
      *frm_lst = newfp_str;
    }

    else{
      struct framephy_struct *traverse = *frm_lst;
      while(traverse->fp_next != NULL){
        traverse = traverse->fp_next;
      }
      traverse->fp_next = newfp_str;
    }
  }

  return 0;
}

/*
 * vm_map_ram - do the mapping all vm are to ram storage device
 * @caller    : caller
 * @astart    : vm area start
 * @aend      : vm area end
 * @mapstart  : start mapping point
 * @incpgnum  : number of mapped page
 * @ret_rg    : returned region
 */
int vm_map_ram(struct pcb_t *caller, int astart, int aend, int mapstart, int incpgnum, struct vm_rg_struct *ret_rg)
{
  struct framephy_struct *frm_lst = NULL;
  int ret_alloc;

  /* @bksysnet: author provides a feasible solution of getting frames
   * FATAL logic in here, wrong behaviour if we have not enough page
   * i.e. we request 1000 frames meanwhile our RAM has size of 3 frames
   * Don't try to perform that case in this simple work, it will result
   * in endless procedure of swap-off to get frame and we have not provide
   * duplicate control mechanism, keep it simple
   */
  ret_alloc = alloc_pages_range(caller, incpgnum, &frm_lst);

  /* Out of memory */
  if (ret_alloc == -1)
  {
#ifdef MMDBG
    printf("OOM: No free frames in SWAP\n");
#endif
    return -1;
  }

  /* it leaves the case of memory is enough but half in ram, half in swap
   * do the swaping all to swapper to get the all in ram */
  vmap_page_range(caller, mapstart, incpgnum, frm_lst, ret_rg);

  print_pgtbl(caller, 0, -1);
  return 0;
}

/* Swap copy content page from source frame to destination frame
 * @mpsrc  : source memphy
 * @srcfpn : source physical page number (FPN)
 * @mpdst  : destination memphy
 * @dstfpn : destination physical page number (FPN)
 */
int __swap_cp_page(struct memphy_struct *mpsrc, int srcfpn,
                   struct memphy_struct *mpdst, int dstfpn)
{
  int cellidx;
  int addrsrc, addrdst;
  for (cellidx = 0; cellidx < PAGING_PAGESZ; cellidx++)
  {
    addrsrc = srcfpn * PAGING_PAGESZ + cellidx;
    addrdst = dstfpn * PAGING_PAGESZ + cellidx;

    BYTE data;
    MEMPHY_read(mpsrc, addrsrc, &data);
    MEMPHY_write(mpdst, addrdst, data);
  }

  return 0;
}

/*
 * Initialize a empty Memory Management instance
 * @mm:     self mm
 * @caller: mm owner
 */
int init_mm(struct mm_struct *mm, struct pcb_t *caller)
{
  struct vm_area_struct *vma = malloc(sizeof(struct vm_area_struct));

  mm->pgd = malloc(PAGING_MAX_PGN * sizeof(uint32_t));

  /* By default the owner comes with at least one vma */
  vma->vm_id = 1;
  vma->vm_start = 0;
  vma->vm_end = vma->vm_start;
  vma->sbrk = vma->vm_start;
  vma->vm_freerg_list = NULL;

  vma->vm_next = NULL;
  vma->vm_mm = mm; /*point back to vma owner */

  mm->mmap = vma;

  return 0;
}

/*
 * enlist_pgn_node: Enlist page node to the list
 * @plist: page list
 * @pgn: page number
 */
int enlist_pgn_node(struct fifo_node **plist, uint32_t* pte, int fpn)
{
  struct fifo_node *pnode = malloc(sizeof(struct fifo_node));
  pnode->pte = pte;
  //printf("enlist pte address %p\n", pte);
  pnode->fpn = fpn;
  pnode->pg_next = *plist;
  *plist = pnode;

  return 0;
}

/*
 * print_list_fp: print list of free frames
 * @plist: list of free frames
 */
int print_list_fp(struct fifo_node *ifp)
{
  struct fifo_node *fp = ifp;
  if (fp == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (fp != NULL)
  {
    printf("fp[%d]\n", fp->fpn);
    fp = fp->pg_next;
  }
  printf("\n");
  return 0;
}

/*
 * print_list_pgn: print list of region in VM
 * @plist: list of regions
 */
int print_list_rg(struct vm_rg_struct *irg)
{
  struct vm_rg_struct *rg = irg;

  printf("print_list_rg: ");
  if (rg == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (rg != NULL)
  {
    printf("rg[%ld->%ld]\n", rg->rg_start, rg->rg_end);
    rg = rg->rg_next;
  }
  printf("\n");
  return 0;
}

/*
 * print_list_vma: print list of virtual memory area
 * @plist: list of virtual memory area
 */
int print_list_vma(struct vm_area_struct *ivma)
{
  struct vm_area_struct *vma = ivma;

  printf("print_list_vma: ");
  if (vma == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (vma != NULL)
  {
    printf("va[%ld->%ld]\n", vma->vm_start, vma->vm_end);
    vma = vma->vm_next;
  }
  printf("\n");
  return 0;
}

/*
 * print_list_pgn: print list of page number
 * @plist: list of page number
 */
int print_list_pgn(struct fifo_node *ip)
{
  printf("print_list_pgn: ");
  if (ip == NULL)
  {
    printf("NULL list\n");
    return -1;
  }
  printf("\n");
  while (ip != NULL)
  {
    uint32_t pte = *(ip->pte);
    int fpn = PAGING_PTE_FPN(pte);
    printf("page[%d]-\n", fpn);
    ip = ip->pg_next;
  }
  printf("\n");
  return 0;
}

/*
 * print_pgtbl: print page table
 * @caller: page table owner
 * @start: start address
 * @end: end address
 */
int print_pgtbl(struct pcb_t *caller, uint32_t start, uint32_t end)
{
  int pgn_start, pgn_end;
  int pgit;

  if (end == -1)
  {
    pgn_start = 0;
    struct vm_area_struct *cur_vma = get_vma_by_num(caller->mm, 0);
    end = cur_vma->vm_end;
  }
  pgn_start = PAGING_PGN(start);
  pgn_end = PAGING_PGN(end);
  
  printf("print_pgtbl: %d - %d [PID=%d] ", start, end, caller->pid);
  if (caller == NULL)
  {
    printf("NULL caller\n");
    return -1;
  }
  printf("\n");

  for (pgit = pgn_start; pgit < pgn_end; pgit++)
  {
    printf("%08ld: %08x\n", pgit * sizeof(uint32_t), caller->mm->pgd[pgit]);
    //printf("%p: %08x\n", &caller->mm->pgd[pgit], caller->mm->pgd[pgit]);
  }

  return 0;
}

#endif
