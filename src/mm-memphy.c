#include "../include/os-cfg.h"

/*
 * PAGING based Memory Management
 * Memory physical module mm/mm-memphy.c
 */

#include "mm.h"
#include <stdlib.h>
#include <stdio.h>

/*
 *  MEMPHY_mv_csr - move MEMPHY cursor
 *  @mp: memphy struct
 *  @offset: offset
 */
int MEMPHY_mv_csr(struct memphy_struct *mp, int offset)
{
   int numstep = 0;

   mp->cursor = 0;
   while (numstep < offset && numstep < mp->maxsz)
   {
      /* Traverse sequentially */
      mp->cursor = (mp->cursor + 1) % mp->maxsz;
      numstep++;
   }

   return 0;
}

/*
 *  MEMPHY_seq_read - read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_seq_read(struct memphy_struct *mp, int addr, BYTE *value)
{
   if (mp == NULL)
      return -1;

   if (!mp->rdmflg)
      return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   *value = (BYTE)mp->storage[addr];

   return 0;
}

/*
 *  MEMPHY_read read MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @value: obtained value
 */
int MEMPHY_read(struct memphy_struct *mp, int addr, BYTE *value)
{
   if (mp == NULL)
      return -1;

   if (mp->rdmflg){
      //printf("addr: %d\n", addr);
      *value = mp->storage[addr];
      //check storage
      //printf("storage: %d\n", mp->storage[addr]);
   }
   else /* Sequential access device */
      return MEMPHY_seq_read(mp, addr, value);

   return 0;
}

/*
 *  MEMPHY_seq_write - write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_seq_write(struct memphy_struct *mp, int addr, BYTE value)
{

   if (mp == NULL)
      return -1;

   if (!mp->rdmflg)
      return -1; /* Not compatible mode for sequential read */

   MEMPHY_mv_csr(mp, addr);
   mp->storage[addr] = value;

   return 0;
}

/*
 *  MEMPHY_write-write MEMPHY device
 *  @mp: memphy struct
 *  @addr: address
 *  @data: written data
 */
int MEMPHY_write(struct memphy_struct *mp, int addr, BYTE data)
{
   
   if (mp == NULL)
      return -1;

   if (mp->rdmflg){
      //printf("addr: %d\n", addr);
      mp->storage[addr] = data;
   }
   else /* Sequential access device */
      return MEMPHY_seq_write(mp, addr, data);

   return 0;
}

/*
 *  MEMPHY_format-format MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_format(struct memphy_struct *mp, int pagesz)
{
   /* This setting come with fixed constant PAGESZ */
   int numfp = mp->maxsz / pagesz;
   struct framephy_struct *newfst, *fst;
   int iter = 0;

   if (numfp <= 0)
      return -1;

   /* Init head of free framephy list */
   fst = malloc(sizeof(struct framephy_struct));
   fst->fpn = iter;

   pthread_mutex_lock(&mp->lock);
   mp->free_fp_list = fst;
   pthread_mutex_unlock(&mp->lock);

   
   /* We have list with first element, fill in the rest num-1 element member*/
   for (iter = 1; iter < numfp; iter++)
   {
      newfst = malloc(sizeof(struct framephy_struct));
      newfst->fpn = iter;
      newfst->fp_next = NULL;
      fst->fp_next = newfst;
      fst = newfst;
   }

   return 0;
}

/*
 *  MEMPHY_get_freefp - get free frame
 *  @mp: memphy struct
 *  @retfpn: return frame number
 */
int MEMPHY_get_freefp(struct memphy_struct *mp, int *retfpn)
{
   pthread_mutex_lock(&mp->lock);
   struct framephy_struct *fp = mp->free_fp_list;
   pthread_mutex_unlock(&mp->lock);

   if (fp == NULL) return -1;
   
   *retfpn = fp->fpn;
   mp->free_fp_list = fp->fp_next;
   
   free(fp);
   
   return 0;
}

/*
 *  MEMPHY_dump - dump MEMPHY device
 *  @mp: memphy struct
 */
int MEMPHY_dump(struct memphy_struct *mp)
{
   if (mp == NULL || mp->storage == NULL)
   {
      return -1; // invalid argument or empty memory region
   }

   printf("Memory dump:\n");

   int i;
   for (i = 0; i < mp->maxsz; i += 4){
      if(mp->storage[i] != 0) printf("%d: %d \n", i, mp->storage[i]);
   }

   return 0;
}

/*
 *  MEMPHY_put_freefp - put free frame
 *  @mp: memphy struct
 *  @fpn: frame number
 */
int MEMPHY_put_freefp(struct memphy_struct *mp, int fpn)
{
   pthread_mutex_lock(&mp->lock);
   struct framephy_struct *fp = mp->free_fp_list;
   pthread_mutex_unlock(&mp->lock);

   struct framephy_struct *newnode = malloc(sizeof(struct framephy_struct));

   /* Create new node with value fpn */
   newnode->fpn = fpn;
   newnode->fp_next = fp;

   pthread_mutex_lock(&mp->lock);
   mp->free_fp_list = newnode;
   pthread_mutex_unlock(&mp->lock);

   return 0;
}

/*
 *  init_memphy - init MEMPHY device
 *  @mp: memphy struct
 *  @max_size: max size of memory
 *  @randomflg: random flag
 */
int init_memphy(struct memphy_struct *mp, int max_size, int randomflg)
{
   mp->storage = (BYTE *)calloc(max_size, sizeof(BYTE));
   mp->maxsz = max_size;

   pthread_mutex_init(&mp->lock, NULL);
   pthread_mutex_init(&mp->fifo_lock, NULL);
   MEMPHY_format(mp, PAGING_PAGESZ);
   mp->fifo_fp_list = NULL;

   mp->rdmflg = (randomflg != 0) ? 1 : 0;

   if (!mp->rdmflg) /* Not Ramdom access device, then it serial device */
      mp->cursor = 0;

   return 0;
}
