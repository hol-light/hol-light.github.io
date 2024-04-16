/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1999 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: July 1999 */

/* Store Management Routines */

#include "ehdr.h"

/* Debugging flags that can be set */

/* #define sanity */
/* #define TraceStore */
/* #define FullTraceStore */

#ifdef __BORLANDC__
#ifdef TraceStore
#include <alloc.h>
#endif
#endif

/* If a macro sys_malloc is not defined, we just use the
normal malloc call, and likewise for sys_free. */

#ifndef sys_malloc
#define sys_malloc malloc
#endif

#ifndef sys_free
#define sys_free   free
#endif

/* Store is taken from the system in blocks whose minimum
size is parameterized here, if not defined elsewhere. This
block size must be a multiple of sizeof(freeblock), which is
typically 8 bytes in 32-byte systems. However, for the DEC
Alpha is is 12 bytes because pointers are 8 bytes long. We
make the default suitable for both. Note that the default can
be over-ridden in a system-dependent header file. */

#ifndef store_allocation_unit
#define store_allocation_unit    30*1024L
#endif

/* Free queue entries start with a pointer to the next entry
and their length. We have to have them in that order, in case
the pointer needs a sticter alignment than an int (e.g. the
DEC Alpha. */

typedef struct {
  void HUGE *free_block_next;
  LONGINT free_block_length;
} freeblock;    

/* Blocks that are passed out and returned start off with just
their length at the start; the address given/received from
the callers is that of the byte after the length. */

typedef struct {
  LONGINT block_length;
} block;   



/*************************************************
*                  Static data                   *
*************************************************/

static freeblock HUGE *store_anchor;
static freeblock HUGE *store_freequeue;


/*************************************************
*         Free queue sanity check                *
*************************************************/

#ifdef sanity
/* Free queue sanity check */

void store_freequeuecheck(void)
{
freeblock HUGE *p = store_freequeue->free_block_next;
while (p != NULL) 
  {
  freeblock HUGE *next = (freeblock HUGE *)(((char HUGE *)p) + p->free_block_length);
  if (p->free_block_next != NULL && next > p->free_block_next)
    {
    debug_printf("OVERLAP %d %d %d %d\n", p, p->free_block_length, next, 
      p->free_block_next);
    }  
  p = p->free_block_next; 
  } 
}
#endif



/*************************************************
*                Initialize                      *
*************************************************/

/* A dummy first entry on the free queue is created. This is
used solely as a means of anchoring the queue. Searches always
start at the block it points to. */

void store_init(void)
{
store_anchor = NULL;
store_freequeue = (freeblock HUGE *)malloc(sizeof(freeblock));
store_freequeue->free_block_next = NULL;
store_freequeue->free_block_length = sizeof(freeblock);
}




/*************************************************
*               Free all store                   *
*************************************************/

/* Blocks of store obtained from the OS are chained together
through their first word(s). This procedure is called when a new
file is to be processed. */

void store_free_all(void)
{
while (store_anchor != NULL)
  {
  freeblock HUGE *p = store_anchor;
  store_anchor = store_anchor->free_block_next;
  sys_free(p);
  }
store_freequeue->free_block_next = NULL;
store_freequeue->free_block_length = sizeof(freeblock);

#ifdef sys_free_all
sys_free_all();
#endif

main_storetotal = 0;
}





/*************************************************
*               Get block                        *
*************************************************/

void *store_get(LONGINT bytesize)
{
int blockrem;
LONGINT newlength;
LONGINT truebytesize;
freeblock HUGE *newblock;
freeblock HUGE *previous = store_freequeue;
freeblock HUGE *p = previous->free_block_next;

#ifdef FullTraceStore
freeblock HUGE *pdebug = previous->free_block_next;
#endif

/* Add space for an initial block to hold the length, and ensure that
the size is a multiple of sizeof(freeblock) so that blocks can
be amalgamated when freed. */

truebytesize = bytesize + sizeof(block);
blockrem = truebytesize % sizeof(freeblock);
if (blockrem != 0) truebytesize += sizeof(freeblock) - blockrem;

#ifdef sanity
store_freequeuecheck();
#endif

/* Keep statistics */

main_storetotal += truebytesize;

/* Search free queue for a block that is big enough */

#ifdef FullTraceStore
while (pdebug != NULL)
{ debug_printf("G1    %8p %8p %8ld\n", pdebug, pdebug->free_block_next, pdebug->free_block_length);
  pdebug = pdebug->free_block_next;
}
#endif

while(p != NULL)
  {
  if (truebytesize <= p->free_block_length)
    {    /* found suitable block */
    block HUGE *pp = (block HUGE *)p;
    LONGINT leftover = p->free_block_length - truebytesize;
    if (leftover == 0)
      {  /* block used completely */
      previous->free_block_next = p->free_block_next;
      }
    else
      {  /* use bottom of block */
      freeblock HUGE *remains = (freeblock HUGE *)(((char HUGE *)p) + truebytesize);
      remains->free_block_length = leftover;
      remains->free_block_next = p->free_block_next;
      previous->free_block_next = remains;
      }

    #ifdef sanity
    store_freequeuecheck();
    #endif
    #ifdef TraceStore
    debug_printf("Get  %5ld %8ld %8p %8ld\n", truebytesize, main_storetotal,pp,leftover);
    #endif  
     
    pp->block_length = truebytesize;  
    #ifdef FullTraceStore
    pdebug = store_freequeue->free_block_next;
    while (pdebug != NULL)
    { debug_printf("G2    %8p %8p %8ld\n", pdebug, pdebug->free_block_next, pdebug->free_block_length);
      pdebug = pdebug->free_block_next;
    }
    #endif
    return (void *)(pp + 1);    /* leave length block hidden */
    }
  else
    {    /* try next block */
    previous = p;
    p = p->free_block_next;
    }
  }

/* no block long enough has been found */

main_storetotal -= truebytesize;  /* correction */

newlength = (truebytesize + sizeof(freeblock) > (LONGINT)store_allocation_unit)?
  truebytesize + sizeof(freeblock) : (LONGINT)store_allocation_unit;

#ifdef store_allocation_unit_is_max
if (newlength > (LONGINT)store_allocation_unit) return NULL;
#endif

#ifdef __BORLANDC__
#ifdef TraceStore
debug_printf("Coreleft=%8ld\n",farcoreleft());
#endif
#endif

newblock = (freeblock HUGE *)sys_malloc(newlength);

/* If malloc fails, return NULL */

if (newblock == NULL)
  {
  return NULL;
  }

/* malloc succeeded */

else
  {
  block HUGE *newbigblock = (block HUGE *)(newblock + 1);

  newblock->free_block_next = store_anchor;           /* Chain blocks through their */
  store_anchor = newblock;                            /* first block */
  newlength -= sizeof(freeblock);

  newbigblock->block_length = newlength;              /* Set block length */
  main_storetotal += newlength;                       /* Correction */
  store_free(newbigblock + 1);                        /* Add to free queue */
  return store_get(bytesize);                         /* Try again */
  }
}



/*************************************************
*     Get store, failing if none available       *
*************************************************/

void *store_Xget(LONGINT bytesize)
{
void *yield = store_get(bytesize);
if (yield == NULL) error_moan(1, bytesize);
return yield;
}



/*************************************************
*          Get a line buffer                     *
*************************************************/

void *store_getlbuff(LONGINT size)
{
linestr *line = store_Xget(sizeof(linestr));
char *text = (size == 0)? NULL : store_Xget(size);
line->prev = line->next = NULL;
line->text = text;
line->key = line->flags = 0;
line->len = size;
return line;
}


/*************************************************
*      Copy store, failing if no store           *
*************************************************/

void *store_copy(void *p)
{
if (p == NULL) return NULL; else
  {
  LONGINT length = ((block HUGE *)p-1)->block_length - sizeof(block);
  void *yield = store_Xget(length);
  memcpy(yield, p, (unsigned)length);
  return yield;
  }
}


/*************************************************
*        Copy a line, failing if no store        *
*************************************************/

linestr *store_copyline(linestr *line)
{
linestr *yield = store_getlbuff(line->len);
char *yieldtext = yield->text;
if (yield == NULL) return NULL;
memcpy((void *)yield, (void *)line, sizeof(linestr));
yield->prev = yield->next = NULL;
yield->text = yieldtext;
if (line->len > 0) memcpy(yield->text, line->text, line->len);
return yield;
}


/*************************************************
*        Copy string, failing if no store        *
*************************************************/

char *store_copystring(char *s)
{
if (s == NULL) return NULL; else
  {
  char *yield = store_Xget(strlen(s)+1);
  strcpy(yield, s);
  return yield;
  }
}



/*************************************************
*    Copy start/end string, failing if no store  *
*************************************************/

char *store_copystring2(char *f, char *t)
{
LONGINT n = t - f;
char *yield = store_Xget(n + 1);
memcpy(yield, f, (unsigned)n);
yield[(unsigned)n] = 0;
return yield;
}



/*************************************************
*           Free a chunk of store                *
*************************************************/

/* The length is in the first word of the block, which is
before the address that the client was given. If the argument
is NULL, do nothing (used for the contents of empty lines). */

void store_free(void *address)
{
LONGINT length;
freeblock HUGE *previous, HUGE *this, HUGE *start, HUGE *end;
#ifdef FullTraceStore
freeblock HUGE *pdebug = store_freequeue->free_block_next;
#endif

if (address == NULL) return;
#ifdef FullTraceStore
while (pdebug != NULL)
{ debug_printf("F1    %8p %8p %8ld\n", pdebug, pdebug->free_block_next, pdebug->free_block_length);
  pdebug = pdebug->free_block_next;
}
#endif

previous = store_freequeue;
this = previous->free_block_next;

start = (freeblock HUGE *) (((block HUGE *)address) - 1);
length = ((block HUGE *)start)->block_length;
end = (freeblock HUGE *)((char HUGE *)start + length);

main_storetotal -= length;

#ifdef sanity
store_freequeuecheck();
#endif

#ifdef TraceStore
debug_printf("Free %5ld %8ld %8p\n", length, main_storetotal, start);
#endif

/* find where to insert */

while (this != NULL)
  {
  if (start < this) break;
  previous = this;
  this = previous->free_block_next;
  }

/* insert */

previous->free_block_next = start;
start->free_block_next = this;
start->free_block_length = length;

/* check for overlap with next */

if (end > this && this != NULL)
  {
  #ifdef TraceStore
  debug_printf("Upwards overlap: start=%8d length=%5d next=%8d\n", start, length, this);
  #endif 
  error_moan(2, start, length, this);
  } 

/* check for contiguity with next */

if (end == this)
  {
  start->free_block_next = this->free_block_next;
  start->free_block_length += this->free_block_length;
  }

/* check for overlap/contiguity with previous */

if (previous != store_freequeue)
  {
  freeblock HUGE *prevend = (freeblock HUGE *)(((char HUGE *)previous) + previous->free_block_length);
  if (prevend > start)
    { 
    #ifdef TraceStore
    debug_printf("Downwards overlap: start=%8d length=%5d next=%8d\n", previous, length, start);   
    #endif 
    error_moan(3, previous, previous->free_block_length, start);
    } 

  if (prevend == start)
    {
    previous->free_block_next = start->free_block_next;
    previous->free_block_length += start->free_block_length;
    }
  }

#ifdef sanity
store_freequeuecheck();
#endif
#ifdef FullTraceStore
pdebug = store_freequeue->free_block_next;
while (pdebug != NULL)
{ debug_printf("F2    %8p %8p %8ld\n", pdebug, pdebug->free_block_next, pdebug->free_block_length);
  pdebug = pdebug->free_block_next;
}
#endif
}


/*************************************************
*       Reduce the length of a chunk of store    *
*************************************************/

void store_chop(void *address, LONGINT bytesize)
{
int blockrem;
LONGINT freelength;
block HUGE *start, HUGE *end;

start = ((block HUGE *)address) - 1;

/* Round up new length as for new blocks */

bytesize += sizeof(block);
blockrem = bytesize % sizeof(freeblock);
if (blockrem != 0) bytesize += sizeof(freeblock) - blockrem;

#ifdef TraceStore
debug_printf("Chop %5ld %8ld\n", bytesize, start);
#endif

/* Compute amount to free, and don't bother if less than four freeblocks */

freelength = start->block_length - bytesize;
if (freelength < 4*sizeof(freeblock)) return;

/* Set revised length into what remains, create a length for the
bit to be freed, and free it via the normal function. */

start->block_length = bytesize;
end = (block HUGE *)(((char HUGE *)start) + bytesize);
end->block_length = freelength;
store_free(end + 1);
}

/* End of estore.c */
