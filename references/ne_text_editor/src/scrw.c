/*************************************************
*                      SCRW                      *
*************************************************/

/* SCRW is a set of routines for displaying text in a scrolling window.
It has some things in common with Acorn's txt routines, but is simpler
and more light-weight. It uses the xs library of store handling functions,
which should be compiled as part of the application. */

/* Philip Hazel, November 1993 */
/* Last modified: November 1993 */

/* For use with NE we cut out the xs interface, since all store is handled
by malloc at the moment. */

#define NO_XS

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "roshdr.h"

#define FALSE 0
#define TRUE  1


#define initial_lastrow 9


/*************************************************
*               Base structure                   *
*************************************************/

typedef struct scrw {
  struct scrw *next;
  int handle;                 /* Handle of the window */
  int width;                  /* Window width in chars */
  int row;                    /* Row number where currently writing */
  int rowstart;               /* Offset to data for current row */
  int col;                    /* Columns where currently writing */
  int offset;                 /* Offset in current line's data */
  int top;                    /* Top of store used */
  int toprow;                 /* Number of the last row */ 
  int size;                   /* Current size of store block */
  int base_size;              /* Original base size */ 
  int inc_size;               /* Increment size for increasing store block */
  char store_type;            /* Malloc only, xs only, or try both */
  char background_colour;     /* Background for this window */
  short int flags;  
  char *buffer;               /* Can be an xs pointer; use only via this field */
} scrw;   

#define scrw_f_malloc     1   /* Indicates buffer was malloc'ed */
#define scrw_f_cr         2   /* Carriage return pending */
#define scrw_f_visbell    4   /* Visible bell */



/*************************************************
*              Local variables                   *
*************************************************/

static scrw *anchor = NULL;


/*************************************************
*          Find an existing window               *
*************************************************/

static scrw *scrw_find(int handle, int errorflag)
{
scrw *w = anchor;
while (w != NULL)
  {
  if (w->handle == handle) return w;
  w = w->next;
  }
if (errorflag) display_error("Internal error: bad window handle for scrw"); 
return NULL;
}


/*************************************************
*            Find character pointer              *
*************************************************/

/* This function finds a character pointer, given a row and column
in the virtual window that is formed by the workspace. If the row
number is greater than the number of lines available, we return a
pointer to an empty line (in fact, the end of the last line). */

static int find_cp(scrw *w, int row, int col)
{
int cp = 0;

while (row-- > 0)
  {
  cp += w->buffer[cp];
  if (cp >= w->top) return cp - 2;
  } 
  
for (;;)
  {
  int c = w->buffer[++cp];
  while (1 <= c && c <= 31) c = w->buffer[++cp];
  if (c == 0 || col-- <= 0) break;
  }  

return cp;
}



/*************************************************
*             Redraw rectangles                  *
*************************************************/

/* This function is called by redraw_window() and update_window()
to go through the rectangles and do the necessary. It need only
clear the rectangles if the flag is set. */

static void redraw_rectangles(scrw *w, int more, window_redraw_block *r, int clear)
{
char buff[256];

while (more)
  {
  int y; 
  int minx = r->gminx - (r->minx - r->scrollx);      /* Calculate the rectangle to be */
  int maxx = r->gmaxx - (r->minx - r->scrollx);      /* updated in workarea coordinates. */
  int miny = r->gminy - (r->maxy - r->scrolly);    
  int maxy = r->gmaxy - (r->maxy - r->scrolly);
  
  if (maxx > w->width * char_width + 4) maxx = w->width * char_width;
  
  /* Loop for all the character rows that need updating, starting from the
  one that sticks down into the top of the rectangle. The variables x and
  y are in workarea coordinates. */  
  
  for (y = maxy + abs(maxy)%char_height; y > miny; y -= char_height)
    {
    char *p = buff;
    int x = (minx-4) - (minx-4)%char_width;
    
    /* Calculate the screen coordinates of the top left-hand corner of the
    first character in the row. */  

    int xa = x + (r->minx - r->scrollx);
    int ya = y + (r->maxy - r->scrolly);
    
    /* Calculate the row and column within the virtual window */
     
    int row = abs(y)/char_height;
    int col = x/char_width;
    
    /* Find the offset to the first character in the buffer */
    
    int cp = find_cp(w, row, col);
    
    /* Clear the rectangle to the background colour if necessary. The flag is set
    after user-initiated re-draws, not after wimp-initiated re-draws. This clearing
    may be a waste if an entire line is in inverse video, but it is simpler to 
    operate this way. */  
    
    if (clear)
      {
      SWI(Wimp_SetColour, 1, w->background_colour + 0x80);
      *p++ = 25;
      *p++ = 4;
      *p++ = r->gminx;
      *p++ = r->gminx >> 8;
      *p++ = ya;
      *p++ = ya >> 8;
      *p++ = 25;
      *p++ = 103;
      *p++ = r->gmaxx;
      *p++ = r->gmaxx >> 8;
      *p++ = (ya - char_height);
      *p++ = (ya - char_height) >> 8;           
      }   
    
    /* Move the current graphics position to the top left character place. We
    adjust all y values by -4 so that the top character in the window doesn't
    get its top pixels chopped off. We adjust all x values by 4 to make a small
    indent. */
     
    *p++ = 25;
    *p++ = 4;
    *p++ = xa + 4;
    *p++ = (xa + 4) >> 8;
    *p++ = ya - 4;     
    *p++ = (ya - 4) >> 8;
    
    /* Add the characters to the VDU buffer, and update the x positions, but
    not if we are at the end of the row's data. */
     
    while (x < maxx)
      {
      if (w->buffer[cp]) *p++ = w->buffer[cp++];
      x += char_width;
      xa += char_width; 
      }   
      
    /* Output remaining queued characters, if any. */
     
    if (p > buff) SWI(OS_WriteN, 2, buff, p - buff);
    } 
  
  /* Try for more rectangles */
   
  more = SWI(Wimp_GetRectangle, 2, 0, (int)r); 
  }
}



/*************************************************
*             Redraw a window                    *
*************************************************/

/* This function is called in response to a Redraw_Window_Request
return from Wimp_Poll. */

void redraw_window(scrw *w)
{
window_redraw_block r;
r.handle = w->handle;
redraw_rectangles(w, SWI(Wimp_RedrawWindow, 2, 0, (int)(&r)), &r, FALSE);
}



/*************************************************
*             Update a window                    *
*************************************************/

/* This function is called when we want to update part of a
window. The rectangle is given in work area coordinates. */

static void update_window(scrw *w, int minx, int miny, int maxx, int maxy)
{
window_redraw_block r;
r.handle = w->handle;
r.minx = minx + 4;
r.miny = miny;
r.maxx = maxx + 4;
r.maxy = maxy;
redraw_rectangles(w, SWI(Wimp_UpdateWindow, 2, 0, (int)(&r)), &r, TRUE);
}




/*************************************************
*          Increase size of a buffer             *
*************************************************/

/* We have to note where the buffer came from, and act accordingly.
If we fail, give an error message, then throw away all the existing
data and re-use the existing buffer. */

static void scrw_increase_buffer(scrw *w)
{
int oldtoprow = w->toprow;

/* Deal with a current malloc'ed block */

if ((w->flags & scrw_f_malloc) != 0)
  {
  char *newblock = realloc(w->buffer, w->size + w->inc_size);
  if (newblock != NULL) 
    {
    w->buffer = newblock;
    w->size += w->inc_size;  
    return;
    }
 
#ifndef NO_XS
  if (w->store_type == scrw_malloc_then_xs || 
      w->store_type == scrw_xs_then_malloc)
    {
    char *oldblock = w->buffer; 
    if (xs_get(w->size + w->inc_size, (void **)(&(w->buffer))) == xs_error_ok)
      {
      memcpy(w->buffer, oldblock, w->size);
      free(oldblock);  
      w->flags &= ~scrw_f_malloc;
      w->size += w->inc_size; 
      return; 
      }  
    }
#endif
     
  }


#ifndef NO_XS
  
/* The existing block comes from xs */

else
  {
  if (xs_adjust(w->size + w->inc_size, (void **)(&(w->buffer))) == xs_error_ok)
    {
    w->size += w->inc_size;
    return;  
    }  
  
  if (w->store_type == scrw_malloc_then_xs || 
      w->store_type == scrw_xs_then_malloc)
    {
    char *newblock = malloc(w->size + w->inc_size);
    if (newblock != NULL)
      {
      memcpy(newblock, w->buffer, w->size);
      xs_free((void **)(&(w->buffer)));
      w->buffer = newblock;
      w->size += w->inc_size;
      w->flags |= scrw_f_malloc;    
      return; 
      }    
    }
  }
#endif  
 
  
/* If we get this far, we have failed to get any more store. After
telling the user, throw away all the text and start again. */

display_error("Ran out of memory for text display window");

w->row = w->toprow = w->rowstart = 0;
w->col = 0;
w->offset = 1;
w->top = 3;
w->buffer[0] = 3;  
w->buffer[1] = 0;
w->buffer[2] = 3;

update_window(w, 0, -(oldtoprow * char_height), w->width * char_width + 4, 0);
} 


/*************************************************
*         Reset to an empty window               *
*************************************************/

/* This involves getting the basic store */

static int scrw_reset_window(scrw *w)
{
w->row = w->rowstart = 0;
w->col = 0;
w->offset = 1;
w->top = 3;
w->toprow = 0;
w->flags = 0;
w->size = w->base_size;

/* Now get the text buffer, either from malloc or xs, as requested */

switch (w->store_type)
  {
  case scrw_malloc_then_xs:
  if ((w->buffer = malloc(w->base_size)) != NULL)
    { 
    w->flags |= scrw_f_malloc;
    break;
    }
  /* Else fall through */  
     
  case scrw_xs_only:  
#ifndef NO_XS 
  if (xs_get(w->base_size, (void **)(&(w->buffer))) != xs_error_ok) 
#endif 
    {
    display_error("No memory for text window"); 
    return FALSE;
    } 
  break;
  
  case scrw_xs_then_malloc: 
#ifndef NO_XS   
  if (xs_get(w->base_size, (void **)(&(w->buffer))) == xs_error_ok) break;
#endif   
  /* Else fall through */ 
  
  case scrw_malloc_only:
  if ((w->buffer = malloc(w->base_size)) == NULL) 
    {
    display_error("No memory for text window"); 
    return FALSE;
    } 
  w->flags |= scrw_f_malloc; 
  break;
  }     

/* Set up a null first line in the buffer */

w->buffer[0] = 3;  
w->buffer[1] = 0;
w->buffer[2] = 3;

return TRUE;
}


    
/*************************************************
*           Initialize a window for text         *
*************************************************/ 

/* The window control block must have been set up by the caller, allowing
adjustment of size, position, title, etc. This routine sets the initial
workarea size, and creates and the window. It returns the window handle or 
-1 if it fails. The window is not opened. */

int scrw_init(window_data *window, int width, int base_size, int inc_size, int store_type)
{
scrw *w = malloc(sizeof(scrw));
if (w == NULL) 
  {
  display_error("No memory for text window base");
  return -1;
  } 

/* Set up the data in the control block that remains, even when the window is
emptied and re-initialized. */

w->width = width;
w->store_type = store_type;
w->background_colour = window->work_background;
w->base_size = base_size;
w->inc_size = inc_size;

/* Get store for the window, and set it up as empty */

if (!scrw_reset_window(w))
  {
  free(w);
  return -1;
  }   

/* Connect the control block to the chain */

w->next = anchor;
anchor = w;

/* Set the workarea size of the window, create it, and return its handle. */

window->work_maxx = width * char_width + 8;
window->work_miny = -((initial_lastrow + 1)* char_height + 4);
return (w->handle = SWI(Wimp_CreateWindow, 2, 0, (int)window));
}



/*************************************************
*      Dispose of a window's data structures     *
*************************************************/

/* The assumption is that the window has been closed already. */

int scrw_delete(int handle)
{
scrw **ww = &anchor;
scrw *w = anchor;
while (w != NULL)
  {
  if (w->handle == handle)
    {
    *ww = w->next;
    if (w->buffer != NULL)
      {  
      if ((w->flags & scrw_f_malloc) != 0) free(w->buffer); 
#ifndef NO_XS
        else xs_free((void **)(&(w->buffer))); 
#endif         
      }   
    free(w);
    return TRUE;  
    }
  ww = &(w->next);
  w = w->next;   
  } 
return FALSE;
}


/*************************************************
*             Empty a window of text             *
*************************************************/

/* This must also reduce the store used back to the
original base size and location. */

int scrw_empty(int handle, int openflag)
{
int block[4];
scrw *w = scrw_find(handle, TRUE);

if (w == NULL) return FALSE;
if ((w->flags & scrw_f_malloc) != 0) free(w->buffer); 

#ifndef NO_XS
  else xs_free((void **)(&(w->buffer))); 
#endif   
  
if (!scrw_reset_window(w))
  {
  w->buffer = NULL; 
  scrw_delete(handle);
  return FALSE; 
  }
  
/* Reset the extent */

block[0] = 0;
block[1] = -((initial_lastrow + 1) * char_height + 4);
block[2] = w->width * char_width + 8;
block[3] = 0; 
SWI(Wimp_SetExtent, 2, w->handle, (int)block);

/* Re-open if requested */

if (openflag)
  {
  window_state_block ws; 
  ws.handle = w->handle;
  SWI(Wimp_GetWindowState, 2, 0, (int)(&ws));  
  ws.scrollx = 0; 
  ws.scrolly = 0;
  SWI(Wimp_OpenWindow, 2, 0, (int)(&ws)); 
  } 
  
return TRUE; 
}



/*************************************************
*        Add data string to text in window       *
*************************************************/

/* This assumes the string is all data characters of the same
rendition, and will fit on the line. */

static void scrw_putdata(scrw *w, char *s)
{
int pos, linelen;
int count = strlen(s);

/* If we are running out of memory, increase the block. Assume
that the block sizes are longer than the window width! */
  
if (w->top + count >= w->size) scrw_increase_buffer(w);

/* Compute the offset to the insertion point */

pos = w->rowstart + w->offset;

/* Handle the insertion of data in the middle of the existing
data - not yet implemented... */

if (pos + 2 < w->top)
  {
  }

/* Now we can copy in the characters, update the pointers,
and cause the relevant bit of window to be redrawn. */
 
memcpy(w->buffer + pos, s, count);
w->top += count;

linelen = count + w->buffer[w->rowstart];
w->buffer[w->rowstart] = linelen;
w->buffer[w->rowstart + linelen - 1] = linelen;
w->buffer[w->rowstart + linelen - 2] = 0;

update_window(w, w->col*char_width, -(w->row+1)*char_height,
  (w->col+count)*char_width, 4 - (w->row*char_height));
w->col += count;
w->offset += count;
}



/*************************************************
*             Display text in window             *
*************************************************/

/* The input string may contain control characters of various kinds */

void scrw_puts(int handle, char *s)
{
int col, c;
char buff[256];
char *p = buff;
scrw *w = scrw_find(handle, TRUE);
if (w == NULL) return;

/* Scan the string, looking for control characters. Normal characters
are packed up into strings for faster screen updating. The variable
col keeps track of where we will be when the packed up string is
output. */

col = w->col;
while ((c = *s++) != 0)
  {
  /* If last character was carriage return not at lhs, and this one is
  not newline, treat the cr as binary. */
  
  if ((w->flags & scrw_f_cr) != 0)
    {
    if (c != '\n') { c = '\r'; s--; } else w->flags &= ~scrw_f_cr;
    }    

  /* If over the screen edge, insert a newline */
    
  if (c >= 32 && c != 127 && col >= w->width) { c = '\n'; s--; }  
 
  /* Handle control characters, first outputting any saved string. Carriage
  returns are ignored when immediately followed by newlines, or when already
  in column 0. */ 
   
  if (c < 32 || c == 127)
    {
    if (p > buff) { *p = 0; scrw_putdata(w, buff); p = buff; col = w->col; }
     
    switch (c)
      {
      case '\n':
      w->rowstart += w->buffer[w->rowstart];
      w->offset = 1;
      w->col = 0;
      w->row++;    

      /* If we are at the end of the data, create a new line. */
       
      if (w->rowstart >= w->top)
        {
        /* Increase workarea depth if required, and scroll to
        the bottom to display the new line. */
         
        if (++(w->toprow) > initial_lastrow)
          {
          window_state_block ws; 
          int block[4];
          block[0] = 0;
          block[1] = -((w->toprow + 1) * char_height + 4);
          block[2] = w->width * char_width + 8;
          block[3] = 0; 
          SWI(Wimp_SetExtent, 2, w->handle, (int)block);
          ws.handle = w->handle;
          SWI(Wimp_GetWindowState, 2, 0, (int)(&ws));  
          ws.scrolly = -999999;
          SWI(Wimp_OpenWindow, 2, 0, (int)(&ws)); 
          } 
        
        /* Increase store size if required */
          
        if (w->rowstart + 3 >= w->size) scrw_increase_buffer(w);
        
        /* Create empty line, except when increasing has failed and
        has reset the buffer. */
           
        if (w->rowstart > 0)
          { 
          w->buffer[w->rowstart] = 3;
          w->buffer[w->rowstart + 1] = 0;
          w->buffer[w->rowstart + 2] = 3;
          w->top += 3; 
          } 
        }  
      break; 
      
      case 7:  /* bel */
      SWI(OS_WriteC, 1, 7); 
      break;
      
      case 8:    /* backspace */
      case 127:  /* delete */ 
      if (w->col > 0) { w->col--; w->offset--; }
      break;
      
      case '\r':
      if ((w->flags & scrw_f_cr) == 0)
        {  
        if (col > 0) w->flags |= scrw_f_cr;
        break;
        }
      w->flags &= ~scrw_f_cr;
      /**/    
      /* Fall through to print as hex */  
      /**/
      default:
      sprintf(buff, "[%X.2]", c);
      scrw_puts(handle, buff);        /* Recursive call */
      break;  
      }  
      
    /* Reset the virtual column for the saved string */
     
    col = w->col;
    }
    
  /* Handle a printing character */
    
  else { *p++ = c; col++; }
  } 
  
/* Output any remaining pending string */

if (p > buff) { *p = 0; scrw_putdata(w, buff); }
}


/*************************************************
*        Handle Wimp_Poll for scrw window        *
*************************************************/

void scrw_poll(int pollaction, int *pollreturn)
{
scrw *w;
switch(pollaction)
  {
  case Redraw_Window_Request:
  if ((w = scrw_find(pollreturn[0], FALSE)) != NULL) redraw_window(w);
  break;   
  }
}

/* End of scrw.c */
