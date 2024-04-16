/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1997 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: June 1997 */


/* This file contains code for making cutting, pasting, and copying
blocks of lines and rectangles. */

#include "ehdr.h"



/*************************************************
*          Cut out or copy a text block          *
*************************************************/

/* A text block is considered to be a string of characters
with included "newlines". It is stored as a chain of line
buffers, each break between buffers signifying the
appearance of a "newline". */

static void cut_text(linestr *startline, linestr *endline, int startlinenumber,
  int startcol, int endcol, BOOL copyflag, BOOL topcurrent)
{
linestr *nextline, *cutline;
int deletecount = 1;
int oldlen = startline->len;
int firstright = (startline == endline)? endcol : oldlen;

if ((startline->flags & lf_eof) != 0) return;

if (startcol > oldlen) startcol = oldlen;
if (firstright > oldlen) firstright = oldlen;

/* Extract data from the first line and either add to the
last existing buffer, or create a new buffer if there are
none already. Existing buffers are present when appending. */

if (cut_last == NULL)
  {
  cutline = line_cutpart(startline, startcol, firstright, copyflag);
  cut_buffer = cutline;
  cut_last = cutline;
  }
else
  {
  line_insertch(cut_last, cut_last->len,
    startline->text + startcol, firstright-startcol, 0);
  if (!copyflag)
    {
    line_deletech(startline,  startcol, firstright-startcol, TRUE);
    startline->flags |= lf_shn;
    }
  }
  
/* If windowing, arrange for the first line to be re-drawn */

#ifdef Windowing
if (main_windowing) sys_window_display_line(startlinenumber, startcol); 
#endif     

/* Now deal with any subsequent complete lines */

if (!copyflag) cursor_col = startcol;    /* final cursor position */
if (startline == endline) return;        /* operation is all on one line */

/* The operation spreads over at least one line end */

nextline = startline->next;
while (nextline != endline && (nextline->flags & lf_eof) == 0)
  {
  linestr *nnextline = nextline->next;

  /* Cut line out of normal chain or copy it */

  if (copyflag) nextline = line_copy(nextline); else
    {
    int i; 
    startline->next = nnextline;
    nnextline->prev = startline;
    main_linecount--; 
    
    /* Remove line from the back list */
    
    for (i = 0; i < back_size; i++)
      {
      if ((main_backlist[i]).line == nnextline)
        {
        int j;
        for (j = i + 1; j < back_size; j++) main_backlist[j-1] = main_backlist[j];
        (main_backlist[back_size-1]).line = NULL;
        if (main_backptr > i) main_backptr--;
        }
      }
    }

  /* Add to cut buffer chain */

  cut_last->next = nextline;
  nextline->prev = cut_last;
  nextline->next = NULL;
  cut_last = nextline;

  /* And carry on */
   
  deletecount ++;
  nextline = nnextline;
  }

/* Now take out the initial part of the final line */

if (endcol > nextline->len) endcol = nextline->len;
cutline = line_cutpart(nextline, 0, endcol, copyflag);
cut_last->next = cutline;
cutline->prev = cut_last;
cut_last = cutline;

/* Now, unless copying, join the two lines on either side of the
split; if there is no data left before the split, cancel that
line. */

if (!copyflag)
  {
  if (startline->len == 0) 
    {
    line_delete(startline, FALSE); 
    main_current = endline; 
    }
  else
    {
    if ((nextline->flags & lf_eof) != 0) main_current = endline;
      else main_current = line_concat(endline, 0);
    }
    
  /* If windowing, arrange for the window to be scrolled up
  to account for deleted lines, and set the current line number
  to be that of the original line. Note that we have only got
  here if the cutting has stretched over at least one line end. */ 
   
  #ifdef Windowing
  if (main_windowing)
    {
    if (!topcurrent) main_currentlinenumber -= deletecount;
    sys_window_deletelines(main_currentlinenumber + 1, deletecount);
    cursor_row = main_currentlinenumber; 
    }
  #endif      
  }
}



/*************************************************
*             Delete text block                  *
*************************************************/

static void cut_deletetext(linestr *startline, linestr *endline,
  int startcol, int endcol, BOOL topcurrent)
{
linestr *nextline;
int deletecount = 1;
int oldlen = startline->len;
int firstright = (startline == endline)? endcol : oldlen;

if ((startline->flags & lf_eof) != 0) return;

if (startcol > oldlen) startcol = oldlen;
if (firstright > oldlen)
  {
  firstright = oldlen;
  startline->flags |= lf_clend;
  }
else startline->flags |= lf_shn;

/* Delete data from first line */

line_deletech(startline, startcol, firstright-startcol, TRUE);

/* Delete any subsequent complete lines */

cursor_col = startcol;               /* final cursor position */
if (startline == endline) return;    /* it all happens on one line */

/* The action spreads over more than one line */

nextline = startline->next;
while (nextline != endline && (nextline->flags & lf_eof) == 0)
  {
  nextline = line_delete(nextline, TRUE);
  deletecount++; 
  }

/* Now take out the initial part of the final line */

if (endcol > nextline->len) endcol = nextline->len;
line_deletech(nextline, 0, endcol, TRUE);
nextline->flags |= lf_shn;

/* Now joint the two lines on either side of the split */

if (startline->len == 0)
  {
  line_delete(startline, FALSE);
  main_current = endline;
  }
else
  {
  if ((nextline->flags & lf_eof) != 0) main_current = endline;
    else main_current = line_concat(endline, 0);
  }
  
/* If windowing, arrange for the window to be scrolled up
to account for deleted lines, and set the current line number
to be that of the original line. Note that we have only got
here if the deletion has stretched over at least one line end. */ 
 
#ifdef Windowing
if (main_windowing)
  {
  if (!topcurrent) main_currentlinenumber -= deletecount;
  sys_window_deletelines(main_currentlinenumber + 1, deletecount);
  cursor_row = main_currentlinenumber; 
  }
#endif      
}



/*************************************************
*             Paste in a text block              *
*************************************************/

/* This procedure is called only when there is at
least one buffer of data in the cut buffer. The yield
is the number of "newlines" inserted. */

int cut_pastetext(void)
{
linestr *line = main_current;
linestr *pline = cut_buffer;
int oldlinecount = main_linecount;
int added;
int current_correction = 0;
BOOL ateof = (line->flags & lf_eof) != 0;

cut_pasted = TRUE;

/* If the cursor is at the left hand side, move line
pointer back to the previous line. If the cursor is not at the
left hand side, insert the first buffer's chars into the current
line, and split if there is more data. */

if (cursor_col == 0) 
  {
  line = line->prev; 
  }  
else
  {
  line_insertch(line, cursor_col, pline->text, pline->len, 0);
  line->flags |= lf_shn;
  cursor_col += pline->len;
  pline = pline->next;

  /* If there are no more buffers of text, we are done. Otherwise,
  split the current line after the first insertion. */

  if (pline == NULL) return 0;

  /* If we were at end of file, the action of inserting characters
  will have split the line already. */

  main_current = ateof? line->next : line_split(line, cursor_col);
  }

/* Now insert all but the last buffer as independent lines, then
insert the last buffer at the start of the new current line. Ensure
that the chain of lines is correct at all times. */

while (pline->next != NULL)
  {
  linestr *nline = line_copy(pline);
  if (line == NULL) main_top = nline; else line->next = nline;
  main_current->prev = nline;
  nline->next = main_current;
  nline->prev = line;
  nline->flags |= lf_shn;
  line = nline;
  pline = pline->next;
  main_linecount++;  
  }

/* Now insert final section of data. However, avoid
adding zero chars to the eof line, as this causes an
unwanted null line to be created. */

if ((main_current->flags & lf_eof) == 0)
  line_insertch(main_current, 0, pline->text, pline->len, 0);
else if (pline->len > 0)
  {
  line_insertch(main_current, 0, pline->text, pline->len, 0);
  current_correction = -1;
  }  

main_current->flags |= lf_shn;
cursor_col = pline->len;

cmd_recordchanged(main_current, cursor_col);

/* Compute the number of "newlines" added */

added = main_linecount - oldlinecount; 

/* If we are windowing, we must scroll down the untouched 
lines in the window. We get here only if at least one 
"newline" has been inserted. If one of the newlines was
added because there was an insertion into the eof line,
then the eof line is not left as the current line and so
main_currentlinenumber is one less than otherwise. */

#ifdef Windowing
if (main_windowing)
  {
  sys_window_insertlines(main_currentlinenumber + 1, added);
  main_currentlinenumber += added + current_correction;  
  }
#endif

return added;
}




/*************************************************
*          Cut out a rectangle of data           *
*************************************************/

static void cut_rect(linestr *startline, linestr *endline, int startcol,
  int endcol, BOOL copyflag)
{
linestr *line = startline;
int left, right;

if (startcol < endcol)
  { left = startcol; right = endcol; }
else
  { left = endcol; right = startcol; }

if (startcol != endcol) for (;;)
  {
  linestr *cutline = line_cutpart(line, left, right, copyflag);
  if (cut_last == NULL) cut_buffer = cutline;
    else cut_last->next = cutline;
  cutline->prev = cut_last;
  cut_last = cutline;
  if (line == endline) break;
  line = line->next;
  }

if (!copyflag) cursor_col = left;
}



/*************************************************
*          Delete a rectangle of data            *
*************************************************/

static void cut_deleterect(linestr *startline, linestr *endline, int startcol,
 int endcol)
{
linestr *line = startline;
int left, right;

if (startcol < endcol)
  { left = startcol; right = endcol; }
else
  { left = endcol; right = startcol; }

if (startcol != endcol) for (;;)
  {
  line_deletech(line, left, right-left, TRUE);
  line->flags |= lf_shn;
  if (line == endline) break;
  line = line->next;
  }

cursor_col = left;
}



/*************************************************
*           Paste in rectangular data            *
*************************************************/

/* This procedure is called only when there is something
in the cut buffer. The cursor is at the top lefthand
corner of the pasted in rectangle. */

void cut_pasterect(void /* LineFlagProc */)
{
linestr *line = main_current;
linestr *pline = cut_buffer;
int maxwidth = 0;

cut_pasted = TRUE;

/* Find maximum width */

while (pline != NULL)
  {
  if (pline->len > maxwidth) maxwidth = pline->len;
  pline = pline->next;
  }

/* Now do the pasting */

pline = cut_buffer;
while (pline != NULL)
  {
  int ateof = (line->flags & lf_eof) != 0;
  int width = pline->len;

  /* Modify the line if there is data, or if it's the eof line (which will
  be converted into a non-eof line and a new eof line invented). */

  if (width > 0 || ateof)
    line_insertch(line, cursor_col, pline->text, width, maxwidth - width);
  line->flags |= lf_shn;

  pline = pline->next;
  line = mac_nextline(line);
  }
}



/*************************************************
*         Check for permitted cut overwrite      *
*************************************************/

BOOL cut_overwrite(char *s)
{
int i;
linestr *line = cut_buffer;
error_moan(28);
error_printf("** The first few lines are:\n");
for (i = 0; i < 3 && line != NULL; i++)
  {
  line_verify(line, FALSE, FALSE);
  line = line->next;
  }
return cmd_yesno(s, 0, 0, 0, 0);
}



/*************************************************
* Cut out, copy out, or delete text or rectangle *
*************************************************/

/* The line and column that were marked are given as
arguments, so that the mark can be deleted before calling
this routine (before anything is disturbed). The type of
item to be cut, a flag for indicating "copy" (as opposed to
"cut"), a flag indicating "delete" (as opposed to "copy" or
"cut") are the other args. The yield is FALSE when the
operation was aborted because of an un-pasted cut buffer. */

BOOL cut_cut(linestr *markline, int markcol, int type,
  BOOL copyflag, BOOL deleteflag)
{
linestr *startline, *endline;
int above = line_checkabove(markline);
int startlinenumber = main_currentlinenumber;
int startcol, endcol;
BOOL topcurrent;

if (!cut_pasted && !deleteflag && !main_appendswitch && cut_buffer != 0 &&
  (cut_buffer->len != 0 || cut_buffer->next != NULL) && main_warnings)
  {
  #ifdef Windowing
  if (main_windowing)
    {
    if (!sys_window_yesno(FALSE, "Continue", 
      "The contents of the cut buffer have not been pasted.\n")) return FALSE;
    }
  else
  #endif
    {       
    if (!cut_overwrite("Continue with CUT or COPY (Y/N)? ")) return FALSE;
    } 
  }

if (!deleteflag) cut_pasted = FALSE;

if (above > 0 || (above == 0 && markcol < cursor_col))
  {
  startline = markline;
  startlinenumber -= above; 
  endline = main_current;
  startcol = markcol;
  endcol = cursor_col;
  topcurrent = FALSE; 
  }
else
  {
  startline = main_current;
  endline = markline;
  startcol = cursor_col;
  endcol = markcol;
  topcurrent = TRUE; 
  }

if (!deleteflag) cut_type = (type == mark_text)? cuttype_text : cuttype_rect;

/* Take appropriate actions for cut/copy/delete */

if (deleteflag)
  {
  if (type == mark_text) cut_deletetext(startline,endline,startcol,endcol,topcurrent);
    else cut_deleterect(startline,endline,startcol,endcol);
  }
else
  {
  /* Delete stuff already in the cut buffer, unless the global
  "append while cutting" switch is set. */

  if (!main_appendswitch)
    {
    while (cut_buffer != NULL)
      {
      linestr *next = cut_buffer->next;
      store_free(cut_buffer->text);
      store_free(cut_buffer);
      cut_buffer = next;
      }
    cut_last = NULL;
    }

  /* Cut rectangle or text */

  if (type == mark_text)
    cut_text(startline,endline,startlinenumber,startcol,endcol,copyflag,topcurrent);
      else cut_rect(startline,endline,startcol,endcol,copyflag);
  }

return TRUE;
}

/* End of ecutcopy.c */
