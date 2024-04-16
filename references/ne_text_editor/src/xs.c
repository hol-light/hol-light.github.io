/*************************************************
*                      XS                        *
*************************************************/

/* XS is a set of routines for managing the store "above" a task's
region in RISC OS. It has some similarity with Acorn's "flex"
routines, but is more lightweight, and much smaller. */

/* Written by Philip Hazel, starting October 1993. */
/* Last modified: November 1993. */

/* This version cut down for use with NE. It provides only
for getting blocks, and for re-setting to the initial state.
Blocks are never freed individually, or moved. Hence there is
no need to keep pointers to them. */


#include <stdio.h>
#include "roshdr.h"

#define Wimp_SlotSize  0x400EC
#define store_offset (32*1024)    /* The slot starts at this address */



/*************************************************
*             Static data for XS                 *
*************************************************/

static int  xs_original_slotsize;       /* The base slotsize */
static int  xs_slotsize;                /* Current slot size */
static int  xs_slotused;                /* Current high water mark */




/*************************************************
*                   Extend heap                  *
*************************************************/

/* This routine is called by the kernel when it wants to extend
the heap after we have pushed up the top of store to get blocks. 
We don't use this in NE. */

static int extendproc(int n, void **p)
{
return 0;
}


/*************************************************
*              Free all xs store                 *
*************************************************/

void xs_free_all(void)
{
SWI(Wimp_SlotSize, 2, xs_original_slotsize, -1);
xs_slotsize = xs_slotused = xs_original_slotsize;
}



/*************************************************
*            Get a new chunk of store            *
*************************************************/

void *xs_get(int size)
{
void *yield;
if (size > xs_slotsize - xs_slotused)
  {
  SWI(Wimp_SlotSize, 2, xs_slotused + size, -1);
  if (swi_regs.r[0] < xs_slotused + size) 
    {
    SWI(Wimp_SlotSize, 2, xs_slotsize, -1);
    return NULL;
    } 
  xs_slotsize = swi_regs.r[0];
  }
yield = (void *)(xs_slotused + store_offset);  
xs_slotused += size;
return yield;
}



/*************************************************
*                Initialize xs                   *
*************************************************/

void xs_init(void)
{
_kernel_register_slotextend(extendproc);
SWI(Wimp_SlotSize, 2, -1, -1);
xs_original_slotsize = xs_slotsize = xs_slotused = swi_regs.r[0];
}

/* End of xs.c */
