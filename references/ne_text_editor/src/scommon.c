/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge 1991, 1992, 1993 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: December 1993 */


/* This file contains screen-handling code for those systems
which process keystrokes individually. It talks in terms of
"windows", which is a total misnomer in this day and age.
What each "window" is is a rectangular area on a terminal's
screen, treated separately by NE (e.g. a text area or an
area for command input). At present, with only one text 
area in this implementation of E, some of this is a bit of
overkill, but it may come in useful one day. */


#include "ehdr.h"
#include "shdr.h"
#include "scomhdr.h"



/*************************************************
*                 Parameters                     *
*************************************************/

#define sc_maxtabs   100
#define sc_maxwindow  10


/*************************************************
*               Structures                       *
*************************************************/

typedef struct {
  int top;
  int bottom;
} windowstr;



/*************************************************
*              Shared variables                  *
*************************************************/

sc_buffstr *sc_buffer = NULL;     /* the screen buffer */

int sc_maxcol;                    /* maximum column */
int sc_maxrow;                    /* maximum row */


/* Interface routines to display control functions */

void  (*sys_w_cls)(int, int, int, int);
void  (*sys_w_flush)(void);
void  (*sys_w_move)(int, int);
void  (*sys_w_rendition)(int);
void  (*sys_w_putc)(int);
void  (*sys_w_hscroll)(int, int, int, int, int);
void  (*sys_w_vscroll)(int, int, int, BOOL);



/*************************************************
*             Static variables                   *
*************************************************/

windowstr  sc_window[sc_maxwindow+1];

sc_buffstr *sc_buffptr;
sc_buffstr *sc_buffwindow;

sc_buffstr sc_space = { ' ', s_r_normal };

int sc_tabs[sc_maxtabs+1];


int sc_col;                /* current column */
int sc_currentwindow;      /* current window */
int sc_rendition;          /* current rendition wanted */
int sc_row;                /* current row */
int sc_setrendition;       /* currently set rendition */
int sc_screenwidth;        /* one plus maxcol */

int sc_windowbottom;       /* parameters of current window */
int sc_windowdepth;
int sc_windowtop;



/*************************************************
*            Force a rendition                   *
*************************************************/

static void forcerendition(int r)
{
sys_w_rendition(r);
sc_setrendition = r;
}



/*************************************************
*           Flush any buffering                  *
*************************************************/

static void scommon_flush(void)
{
if (sc_rendition != sc_setrendition) forcerendition(sc_rendition);
sys_w_flush();
}



/*************************************************
*         Move a row in the data structure       *
*************************************************/

static void moverow(int f, int t)
{
int i;
sc_buffstr *fp = sc_buffwindow + f*sc_screenwidth;
sc_buffstr *tp = sc_buffwindow + t*sc_screenwidth;
for (i = 0; i <= screen_max_col; i++) tp[i] = fp[i];
}



/************************************************
*           Clear line in buffer                *
************************************************/

static void clearrow(int n)
{
int i;
sc_buffstr *p = sc_buffwindow + n*sc_screenwidth;
for (i = 0; i <= screen_max_col; i++) p[i] = sc_space;
}



/************************************************
*           Insert spaces in buffer             *
************************************************/

static void insertspaces(int n, int row, int col, int margin)
{
int i;
sc_buffstr *p = sc_buffwindow + row*sc_screenwidth;
for (i = margin - n; i >= col; i--) p[i+n] = p[i];
for (i = col; i < col + n; i++) p[i] = sc_space;
}


/************************************************
*           Delete chars in buffer              *
************************************************/

static void deletechars(int n, int row, int col, int margin)
{
int i;
sc_buffstr *p = sc_buffwindow + row*sc_screenwidth;
for (i = col + n; i <= margin; i++) p[i-n] = p[i];
for (i = margin - n + 1; i<= margin; i++) p[i] = sc_space;
}


/************************************************
*            Show chars from buffer             *
************************************************/

static void showrow(int srow, int left, int right, int posflag)
{
int i;
sc_buffstr *p = sc_buffwindow + srow*sc_screenwidth + left;
if (right > window_width) right = window_width;

if (!posflag) sys_w_move(left, srow + sc_windowtop);

for (i = left; i <= right; i++)
  {
  int r = p->rend;
  if (r != sc_setrendition) forcerendition(r);
  sys_w_putc((p++)->ch);
  }

sys_w_move(sc_col, sc_row + sc_windowtop);
}



/*************************************************
*              Return Data                       *
*************************************************/

static int scommon_x(void) { return sc_col; }
static int scommon_y(void) { return sc_row; }
static int scommon_maxx(void) { return sc_maxcol; }
static int scommon_maxy(void) { return sc_maxrow; }
static int scommon_window(void) { return sc_currentwindow; }



/*************************************************
*          Move to x, y in window                *
*************************************************/

static void scommon_move(int x, int y)
{
if (x < 0 || x > screen_max_col || y < 0 || y > sc_windowdepth)
  error_moan(4, "Bad x, y coordinates", "scommon_move", x, y,
    sc_currentwindow, screen_max_col, sc_windowdepth);

/* debug_writelog("scommon_move to x=%d y=%d\n", x, y); */

sc_row = y;
sc_col = x;
sc_buffptr = sc_buffwindow + sc_screenwidth*y + x;
sys_w_move(x, y + sc_windowtop);
}



/*************************************************
*            Define Window                       *
*************************************************/

static void scommon_defwindow(int n, int y1, int y2)
{
windowstr *p = sc_window + n;
if (n < 0 || n > sc_maxwindow)
  error_moan(4, "Bad window number", "sc_defwindow", n, y1, y2);    /* Hard */

if (y1 < 0 || y1 > sc_maxrow || y2 < 0 || y2 > sc_maxrow || y1 < y2)
  error_moan(4, "Bad window data", "sc_defwindow", n, y1, y2);  /* Hard */

p->top = y2;
p->bottom = y1;
}



/*************************************************
*            Make Window Current                 *
*************************************************/

static void scommon_selwindow(int n, int x, int y)
{
windowstr *p = sc_window + n;
if (n < 0 || n > sc_maxwindow)
  error_moan(4, "Bad window number", "sc_selwindow", n, 0, 0, 0, 0);  /* Hard */

sc_currentwindow = n;
sc_windowtop = p->top;
sc_windowbottom = p->bottom;
sc_windowdepth = sc_windowbottom - sc_windowtop;

sc_buffwindow = sc_buffer + sc_windowtop*sc_screenwidth;

if (x >= 0) scommon_move(x, y);
}



/*************************************************
*             Set Tabs                           *
*************************************************/

static void scommon_settabs(int *tablist)
{
int *t = sc_tabs;
int c = *tablist++;
if (c > sc_maxtabs) c = sc_maxtabs;
*t++ = c;
while (c--) *t++ = *tablist++;
}



/*************************************************
*             Clear Screen (Window)              *
*************************************************/

static void scommon_cls(void)
{
int i;
for (i = 0; i <= sc_windowdepth; i++) deletechars(screen_max_col+1, i, 0, screen_max_col);
if (sc_rendition != sc_setrendition) forcerendition(sc_rendition);
sys_w_cls(sc_windowbottom, 0, sc_windowtop, screen_max_col);
sc_buffptr = sc_buffwindow;
scommon_move(0, 0);
}



/*************************************************
*             Set Rendition                      *
*************************************************/

static void scommon_rendition(int r)
{
if (r != s_r_normal && r != s_r_inverse)
  error_moan(4, "Bad rendition", "sc_rendition", r, 0, 0, 0, 0);   /* Hard */
sc_rendition = r;
}



/*************************************************
*          Write a character                     *
*************************************************/

static void scommon_putc(int c)
{
if (sc_setrendition != sc_rendition) forcerendition(sc_rendition);
sys_w_putc(c);
sc_buffptr->ch = c;
sc_buffptr->rend = sc_rendition;
if (sc_col < screen_max_col)
  {
  sc_col++;
  sc_buffptr++;
  }
}


/*************************************************
*              Write a string                    *
*************************************************/

static void scommon_printf(char *s, ...)
{
char buff[256];
char *p = buff;
va_list ap;
va_start(ap, s);
vsprintf(buff, s, ap);
va_end(ap);
while (*p) scommon_putc(*p++);
}



/*************************************************
*             Erase line to right                *
*************************************************/

/* Erase right operates to the right of the screen, independent of the
logical window. In fact, E makes no use of windows that are narrower than
the screen at the present time. */

static void scommon_eraseright(void)
{
BOOL moveneeded = TRUE;
BOOL anydone = FALSE;
sc_buffstr *ptr = sc_buffptr;
int i;

for (i = sc_col; i <= sc_maxcol; i++)
  {
  if (ptr->ch == ' ' && ptr->rend == s_r_normal) moveneeded = TRUE; else
    {
    if (!anydone)
      {
      if (sc_setrendition != s_r_normal) forcerendition(s_r_normal);
      anydone = TRUE;
      }
    if (moveneeded)
      {
      sys_w_move(i, sc_row + sc_windowtop);
      moveneeded = FALSE;
      }
    sys_w_putc(' ');
    *ptr = sc_space;
    }
  ptr++;
  }

scommon_move(sc_col, sc_row);
}



/*************************************************
*       Scroll up or down within window          *
*************************************************/

void scommon_vscroll(int bottom, int top, int amount, BOOL deleting)
{
int i;

if (top < 0 || top > bottom || bottom > sc_windowbottom ||
   abs(amount) > bottom - top + 1)
     {
     error_moan(4, "Bad scroll data", "sc_vscroll",
       bottom, top, amount, deleting);                /* Hard */
     }

if (sc_setrendition != s_r_normal) forcerendition(s_r_normal);

sys_w_vscroll(bottom + sc_windowtop, top + sc_windowtop, amount, deleting);

if (amount > 0)
  {
  for (i = bottom - amount; i >= top; i--)  moverow(i, i + amount);
  for (i = top; i < top + amount; i++) clearrow(i);
  }
else
  {
  amount = -amount;
  for (i = top + amount; i <= bottom; i++) moverow(i, i-amount);
  for (i = bottom - amount + 1; i <= bottom; i++) clearrow(i);
  }

sys_w_move(sc_col, sc_row + sc_windowtop);
}



/*************************************************
*       Scroll left or right within window       *
*************************************************/

static void scommon_hscroll(int left, int bottom, int right, int top, int amount)
{
int i;

if (top < 0 || top > bottom || left < 0 || left > right ||
   bottom > sc_windowbottom || right > screen_max_col ||
   abs(amount) > right - left + 1)
     {
     error_moan(4, "Bad scroll data", "sc_hscroll",
       left, bottom, right, top, amount);              /* Hard */
     }

if (sc_setrendition != s_r_normal) forcerendition(s_r_normal);

if (amount > 0)
  for (i = top; i <= bottom; i++) insertspaces(amount, i, left, right);
else for (i = top; i <= bottom; i++) deletechars(-amount, i, left, right);

if (sys_w_hscroll != NULL)
  sys_w_hscroll(left, bottom + sc_windowtop, right, top + sc_windowtop, amount);

else for (i = top; i <= bottom; i++) showrow(i, left, right, FALSE);
}



/*************************************************
*          Enable/disable overstriking           *
*************************************************/

/* There is nothing specific to be done for this driver */

void scommon_overstrike(int flag)  { return; }



/*************************************************
*                Initialize                      *
*************************************************/

static void scommon_init(int maxrow, int maxcol, BOOL clsflag)
{
int i;

sc_maxrow = maxrow;
sc_maxcol = maxcol;
sc_screenwidth = maxcol + 1;

sc_buffer = store_Xget((maxrow+1)*(maxcol+1)*sizeof(sc_buffstr));

scommon_defwindow(0, maxrow, 0);
scommon_selwindow(0, -1, -1);

for (i = 0; i <= sc_windowdepth; i++) deletechars(screen_max_col+1, i, 0, screen_max_col);
if (clsflag)
  {
  if (sc_rendition != sc_setrendition) forcerendition(sc_rendition);
  sys_w_cls(sc_windowbottom, 0, sc_windowtop, screen_max_col);
  }

sc_buffptr = sc_buffwindow;
scommon_move(0, 0);
}



/*************************************************
*                Terminate                       *
*************************************************/

static void scommon_terminate(void)
{
if (sc_buffer != NULL)
  {
  store_free(sc_buffer);
  sc_buffer = NULL;
  }
}



/*************************************************
*            Select these routines               *
*************************************************/

void scommon_select(void)
{
s_cls = scommon_cls;
s_defwindow = scommon_defwindow;
s_eraseright = scommon_eraseright;
s_flush = scommon_flush;
s_hscroll = scommon_hscroll;
s_init = scommon_init;
s_maxx = scommon_maxx;
s_maxy = scommon_maxy;
s_move = scommon_move;
s_overstrike = scommon_overstrike;
s_printf = scommon_printf;
s_putc = scommon_putc;
s_rendition = scommon_rendition;
s_selwindow = scommon_selwindow;
s_settabs = scommon_settabs;
s_terminate = scommon_terminate;
s_vscroll = scommon_vscroll;
s_window = scommon_window;
s_x = scommon_x;
s_y = scommon_y;
}

/* End of scommon.c */
