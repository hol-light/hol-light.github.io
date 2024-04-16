/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1999 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: July 1999 */


/* This file contains code for displaying a screenful of lines */


#include "ehdr.h"
#include "shdr.h"
#include "scomhdr.h"

/* #define logging          Set to cause logging info to file */





/*************************************************
*              Static variables                  *
*************************************************/

static int  scrn_hscrollamount = 20;
static int  scrn_tryabove = -1;

static linestr *scrn_topline;


/*************************************************
*          Hint Entry from Main Program          *
*************************************************/

/* This procedure is called by

(1) Commands which add or delete lines from the file, to enable
the screen controller take note. All that is done at present is
to disable the "keep the current line at the same place" logic
for inserts near the top of the screen. The general scrolling code
does the rest automatically.

(2) The buffer changing procedure, to put the same line at the top
of the screen as was there when the buffer was last used.

(3) The TOPLINE command, to put the given line at the screen top,
if possible.

(4) The GLOBAL commands, to specify a smaller number than usual
for the lines to go above the current line.
*/

void scrn_hint(int option, int count, linestr *line)
{
#ifdef Windowing
if (!main_windowing)
#endif

switch (option)
  {
  case sh_insert:
    {
    int i;
    int row = -1;
    BOOL eoflastline = FALSE;

    for (i = 0; i < window_depth; i++)
      {
      line = window_vector[i];
      if (line == main_current) row = i;
      if (line != NULL && (int)line > 1) eoflastline = ((line->flags & lf_eof) != 0);
      }

    if (row < 0) return;                /* current line off screen */
    if (eoflastline || (row < window_depth/2 && row < count))
      screen_autoabove = FALSE;
    }
  break;

  case sh_topline:
  scrn_topline = line;
  break;

  case sh_above:
  scrn_tryabove = count;
  break;
  }
}




/*************************************************
*         Write graticule                        *
*************************************************/

/* The interactions of all the flags is a horrible inherited mess.
One day this should all be cleaned up a lot! */

static void graticule(int row, BOOL wipeflag, char *name,
  int namelen, BOOL indflag, BOOL marginflag)
{
s_move(0, row);
s_rendition(s_r_inverse);

/* If wipeflag is set, then indflag is also always set, but not
vice versa. */

if (indflag)
  {
  if (wipeflag) s_eraseright(); 
  s_move(7, row);
  if (main_bufferchain->next != 0)
    s_printf("[%d] ", currentbuffer->bufferno);
  if (currentbuffer->to_fid != NULL) s_printf("S"); 
  s_printf("%c%c%c%c  ", main_readonly? 'R' : (main_overstrike? 'O':'I'),
    (main_appendswitch? 'A':'R'), (cmd_casematch? 'V':'U'),
      (main_AutoAlign? 'A':' '));
  }


if (wipeflag)
  {
  int i;

  if (name != NULL)
    {
    for (i = 0; i <= screen_max_col-23; i++)
      {
      if (i >= namelen) break;
      s_putc(name[i]);
      }
    if (namelen > screen_max_col-23) s_printf("...");
    s_putc(' ');
    } 

  for (i = cursor_offset + 1 + s_x(); i <= cursor_offset + 1 + window_width; i++)
    {
    int j = i;
    if (j == main_rmargin+1) s_putc('|');
    else if (j == (main_rmargin-MAX_RMARGIN)+1) s_putc('\\');
    else if (j%10 == 0 && j < MAX_RMARGIN)
      {
      int n;
      char b[8];
       
      #ifdef NO_PERCENT_N
      sprintf(b, "%d", j/10);
      n = strlen(b); 
      #else  
      sprintf(b, "%d%n", j/10, &n);
      #endif 
       
      s_move(s_x() - n + 1, row);
      s_printf(b);    
      }
    else s_putc((j%5) == 0? '*' : '-');
    }
  }

else if (marginflag)
  {
  int c = (main_rmargin > MAX_RMARGIN)? '\\' : '|';
  int x = main_rmargin;
  if (x > MAX_RMARGIN) x -= MAX_RMARGIN;
  x -= cursor_offset;
  if (0 <= x  && x <= window_width)
    {
    s_move(x, row);
    s_putc(c);
    }
  }

s_rendition(s_r_normal);
}


/*************************************************
*             Display line number                *
*************************************************/

void scrn_display_linenumber(void)
{
char buff[40];
if (main_current->key > 0) sprintf(buff, "%-6d ", main_current->key);
  else sprintf(buff, "       ");

#ifdef Windowing
if (main_windowing) sys_window_display_linenumber(buff); else
#endif
  {
  s_selwindow(first_window + 1, 0, 0);
  s_rendition(s_r_inverse);
  s_printf("%s", buff);
  s_rendition(s_r_normal);
  }
}



/*************************************************
*             Invert given char in line          *
*************************************************/

void scrn_invertchars(linestr *line, int row, int col, int count, int flag)
{
int i;
int len = line->len;
char *p = line->text;
if ((line->flags & lf_eof) != 0)
  {
  p = "End of file";
  len = strlen(p);
  if (col < len) flag = !flag; 
  }   
s_move(col - cursor_offset, row);
if (flag) s_rendition(s_r_inverse);
for (i = 1; i <= count; i++) s_putc((col >= len)? ' ' : p[col++]);
if (flag) s_rendition(s_r_normal);
}



/*************************************************
*          Display a single line                 *
*************************************************/

/* The column argument is the absolute column in the line. */

void scrn_displayline(linestr *line, int row, int col)
{
int mcol = (line != NULL && line == mark_line)? mark_col - cursor_offset : -1;
int scol = col - cursor_offset;

#ifdef Windowing
if (main_windowing)
  {
  sys_window_display_line(row, col);
  line->flags &= ~lf_shbits;
  return;
  }
#endif
      

s_move(scol, row);

if (line != NULL)
  {
  if ((line->flags & lf_eof) != 0)
    {
    s_rendition(s_r_inverse);
    s_printf("End of file");
    s_rendition(s_r_normal);
    }

  else
    {
    int i;
    char *p = line->text;
    for (i = col; i < line->len; i++)
      {
      #ifdef Windowing 
      if ((scol == window_width && i < line->len - 1 && !main_windowing) || scol == mcol)
      #else 
      if ((scol == window_width && i < line->len - 1) || scol == mcol)
      #endif 
        {
        s_rendition(s_r_inverse);
        s_putc(p[i]);
        s_rendition(s_r_normal);
        }
      else s_putc(p[i]);
      if (scol++ >= window_width) break;
      }
    }

  line->flags &= ~lf_shbits;
  }

/* if (scol < window_width) s_eraseright(); */

if (scol <= window_width) s_eraseright();

if (line != NULL && scol <= mcol && mcol <= window_width)
  scrn_invertchars(line, row, mark_col, 1, TRUE);
}



/*************************************************
*            Clear past end of line              *
*************************************************/

/* This facility is used mostly for getting rid of
the end-of-file marker when new material is inserted
at the end of a file. */

static void clearendline(linestr *line, int row)
{
int endcol = line->len - cursor_offset;
if (endcol <= window_width)
  {
  s_move(endcol, row);
  s_eraseright();
  }
line->flags &= ~lf_shbits;
}



/*************************************************
*          Adjust Screen Display                 *
*************************************************/

/* The argument is the row on which to place the
current line, i_e. the number of rows of display
to put above it, if possible. */

static void makescreen(int above)
{
linestr *prev = main_current;
linestr *next = main_current;
linestr *top = NULL;
linestr *line;

int row;
int wptr = 0;
int count = window_depth + 2;
int xabove = ((above >= 0)? above :
  (scrn_tryabove >= 0)? scrn_tryabove : window_depth/2) + 1;

scrn_tryabove = -1;

/* The act of displaying of necessity wipes out any command-type input
at the bottom of the screen. We therefore cancel the "pending newline"
switch, because it is interrogated by the error message routines before
checking whether a command is in progress. */

main_pendnl = FALSE;

/* Catch disasters before going further and muddying
the evidence. */

if (main_current == NULL) error_moan(6);    /* Fatal error */

/* Try to position the current line in the middle
of the display. Search up and down for a suitable
number of lines. However, don't leave blank space
at the bottom unnecessarily. */

while (prev != NULL && xabove > 0)
  {
  xabove--;
  count--;
  top = prev;
  prev = prev->prev;
  }

while (next != NULL && count > 0)
  {
  count--;
  next = mac_nextline(next);
  }

if (above < 0)
  {
  while (prev != NULL && count > 0)
    {
    count--;
    top = prev;
    prev = prev->prev;
    }
  }

/* We now try to optimize the screen writing by seeing if any
groups of lines are in fact on the screen, but in the wrong
vertical position. Such groups can be re-positioned by scrolling
to avoid having to re-write them. */

/* This code doesn't give any benefit in some environments so there
is an option to omit it. In some other environments (e.g. xterm windows) it
is vital, in order to get acceptable performance. */

if (!screen_forcecls && screen_use_scroll)
  {
  line = top;

  /* Find first mis-match */

  while (window_vector[wptr] == line && wptr <= window_depth)
    {
    wptr++;
    if (line != NULL) line = line->next;
    }

  /* Now loop, searching for blocks of lines and scrolling
  as appropriate, in either direction. Don't continue at
  end of file, though. */

  row = wptr;   /* position on new screen */
  while (row <= window_depth && wptr <= window_depth && line != NULL)
    {
    int i;
    int w = wptr;
    count = 1;
    while (w <= window_depth)
      {
      if (window_vector[w] == line)

        /* Found one line that matches; search for group
        and update position in old window. */

        {
        int i;
        linestr *nline = line->next;
        int maxcount = window_depth - row + 1;
        for (i = w + 1; i < window_depth; i++)
          {
          if (count >= maxcount) break;
          if (window_vector[i] != nline) break;
          if (nline != NULL) nline = nline->next;
          count++;
          }
        wptr = w + count;    /* New "unused" ptr */

        /* If group is big enough, and scroll is non-zero,
        do a scroll and adjust the vector. Take care to
        get the boundaries of the scroll correct in all cases. */

        if (count >= main_optscroll && row + count != wptr)
          {
          int scrollamount = row - wptr + count;
          int p = wptr - count;
          int topscroll = min(row, p);
          int botscroll = wptr - 1;

#ifdef logging
          debug_writelog("Scroll: row=%d w=%d wptr=%d count=%d\n",
            row,w,wptr,count);
#endif

          if (row+count-1 > botscroll) botscroll = row+count-1;
          s_vscroll(botscroll, topscroll, scrollamount, TRUE);
          if (scrollamount > 0)
            for (i = count - 1; i >= 0; i--)
              {
              window_vector[row+i] = window_vector[p+i];
              window_vector[p+i] = NULL;
              }
          else for (i = 0; i < count - 1; i++)
              {
              window_vector[row+i] = window_vector[p+i];
              window_vector[p+i] = NULL;
              }
          }

        break;       /* from inner loop after found a match */
        }

      w++;
      }              /* end of match-seeking loop */

    row += count;    /* move down screen */
    for (i = 1; i < count && line->next != NULL; i++) line = line->next;
    }
  }

/* Now display the lines as required; note that line can
become NULL if end of file passed. On reaching the current line,
set the cursor row. */


row = 0;
line = top;

while (row <= window_depth)
  {
  if (line == main_current) cursor_row = row;

  if (line != window_vector[row] || (line != NULL && (line->flags & lf_shn) != 0))
    {
#ifdef logging
    debug_writelog("displaying line %d\n", row);
#endif
    scrn_displayline(line, row, cursor_offset);
    }
  else if (line != NULL && (line->flags & lf_clend) != 0)
    {
#ifdef logging
    debug_writelog("clearendline %d\n", row);
#endif
    clearendline(line, row);
    }
  else if (line != NULL && (line->flags & lf_shm) != 0)
    {
    int p = cursor_offset + window_width;
    if (line->len > p)
      {
      int ch = (line->text)[p];
      if (line->len != p+1) s_rendition(s_r_inverse);
      s_move(window_width, row);
      s_putc(ch);
      s_rendition(s_r_normal);
      }
    }

  window_vector[row++] = line;
  if (line != NULL)
    {
    line->flags &= ~lf_shbits;
    line = line->next;
    }
  }
}



/*************************************************
*          Set up to display screen              *
*************************************************/

void scrn_display(BOOL align)
{
BOOL scrolled = FALSE;
BOOL newwindow = main_selectedbuffer ||
  (window_width != min(screen_max_col, main_rmargin-cursor_offset));
int above = -1;
int newoffset = 0;

#ifdef logging
debug_writelog(">>>>Start of scrn_display()\n");
#endif

#ifdef Windowing
if (main_windowing) { sys_window_display(align, TRUE); return; }
#endif

/* Set up the offset so that the current position is visible. There's a fudge
variable that can be set to cause scrolling when the cursor is near the right-
hand side (used by the G commands). */

if (cursor_col < cursor_offset || cursor_col > cursor_max - cursor_rh_adjust)
  {
  while (cursor_col > newoffset + window_width - cursor_rh_adjust) 
    newoffset += scrn_hscrollamount;
  if (cursor_offset != newoffset)
    {
    cursor_offset = newoffset;
    cursor_max = window_width + cursor_offset;
    newwindow = TRUE;
    scrolled = TRUE;
    screen_forcecls = TRUE; 
    }
  }

/* If clear screen was forced, do it. Ensure we are in the whole screen
window, as that is much faster on many systems. */

if (screen_forcecls)
  {
  int i;
  s_selwindow(0, -1, -1);
  s_cls();
  main_drawgraticules = dg_both;
  for (i = 0; i <= window_depth; i++) window_vector[i] = NULL;
  }

/* Now select the text window */

s_selwindow(first_window, -1, -1);

/* Draw graticules as required */

if (main_drawgraticules != dg_none)
  {
  int top = (main_drawgraticules & dg_top) != 0;
  int both = (main_drawgraticules & dg_both) != 0;
  int wipeflag = both | ((main_drawgraticules & dg_bottom) != 0);
  int namelen = (main_filealias == NULL)? 0 : strlen(main_filealias);
  int indflag = wipeflag | ((main_drawgraticules & dg_flags) != 0);
  int marginflag = (main_drawgraticules & dg_margin) != 0;

  if (both || marginflag || top)
    {
    s_selwindow(0, -1, -1);
    graticule(0, both|top, NULL, namelen, FALSE, marginflag);
    }

  if (main_drawgraticules != dg_top)
    {
    s_selwindow(first_window+1, -1, -1);
    graticule(0, wipeflag, main_filealias, namelen, indflag, marginflag);
    }
  main_drawgraticules = dg_none;
  }

/* Update current line number */

scrn_display_linenumber();

/* Compute how many lines to put above current. Scrn_topline
is set after a buffer switch (and in other cases); use it first
for preference. Otherwise, if autoabove is set, arrange to leave
the current line where it is if already on the screen. Otherwise,
let the display routine use its default algorithm (above=-1). After
some operations, scrn_topline may be junk, but the way this is now written
should cope with that. */

s_selwindow(first_window, -1, -1);

if (scrn_topline != NULL)
  {
  int i;
  linestr *line = main_current;
  for (i = 0; i <= window_depth; i++)
    {
    if (line == scrn_topline)  { above = i; break; }
    line = line->prev;
    if (line == NULL) break;
    }
  }
scrn_topline = NULL;

/* If current is on the screen, leave it where it is. If the current 
is not on the screen, but any line above it is on the screen and 
current will be shown if we leave that line alone, then do that. 
This has a good effect after line splitting. */

if (screen_autoabove && above < 0)
  {
  int i, j;
  linestr *line = main_current;
  for (j = 0; above < 0 && line != NULL && j < window_depth - 1; j++)
    {
    for (i = 0; i <= window_depth - j; i++)
      if (window_vector[i] == line) { above = i + j; break; } 
    line = line->prev; 
    }
  }
 
screen_autoabove = TRUE;
makescreen(above);
s_move(cursor_col - cursor_offset, cursor_row);
screen_forcecls = FALSE;

#ifdef logging
debug_writelog("End of scrn_display()\n");
#endif
}

/* End of edisplay.c */
