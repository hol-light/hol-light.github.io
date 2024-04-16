/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 2003 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: March 2003 */


/* This file contains code for making changes to individual lines */

#include "ehdr.h"



/*************************************************
*           Check position of line               *
*************************************************/

/* If the given line is above the current line, the yield is
the count of lines between them; otherwise it is -1. */

int line_checkabove(linestr *line)
{
int count = 0;
linestr *up = main_current;
while (up != line && up != NULL)
  {
  count++;
  up = up->prev;
  }
return (up == NULL)? -1 : count;
}



/*************************************************
*             Make a copy of a line              *
*************************************************/

linestr *line_copy(linestr *line)
{
linestr *nline = store_getlbuff(line->len);
nline->key = line->key;
nline->flags = line->flags;
if (line->len > 0) memcpy(nline->text, line->text, line->len);
return nline;
}



/*************************************************
*           Insert chars into line               *
*************************************************/

/* The characters are inserted at col, the line being extended with
spaces if necessary. Ptr is a pointer count characters to be inserted.
Padcount is a count of spaces to be inserted AFTER the data supplied.

Note that this procedure does NOT set lf_shn in the line. */

void line_insertch(linestr *line, int col, char *ptr, int count, int padcount)
{
int i;
int oldlen = line->len;
int extra = (col > oldlen)? col - oldlen : 0;
int newlen = oldlen + extra + count + padcount;
char *newtext = store_Xget(newlen);
char *np = newtext;
int leftcount = (extra == 0)? col : oldlen;
int rightcount = oldlen - col;

if (rightcount < 0) rightcount = 0;

memcpy(np, line->text, leftcount);
np += leftcount;
for (i = 0; i < extra; i++) *np++ = ' ';
memcpy(np, ptr, count);
np += count;
for (i = 0; i < padcount; i++) *np++ = ' ';
memcpy(np, line->text + col, rightcount);

if (line->text != NULL) store_free(line->text);
line->text = newtext;
line->len = newlen;

/* If we have added data to the end-of-file line,
make a new, null eof line and add it on the end. */

if ((line->flags & lf_eof) != 0)
  {
  main_bottom = store_getlbuff(0);
  line->next = main_bottom;
  main_bottom->prev = line;
  line->flags &= ~lf_eof;
  main_bottom->flags |= lf_eof + lf_shn;
  main_linecount++;

  if (extra == 0)
    {
    line->flags &= ~lf_shn;     /* must have been previous insert */
    line->flags |= lf_clend;    /* clear old eof marker */
    }
  else line->flags |= lf_shn;
  }

/* Sometimes data is added to an eof line on the screen more than
once before the main part of E gets control. We need to make sure
that the whole line is shown if necessary. The following fudge
covers this case. */

if ((line->flags & lf_clend) != 0 && extra > 0) line->flags |= lf_shn;

/* Adjust the mark if necessary */

if (mark_line == line && col <= mark_col) mark_col += count + padcount;
cmd_recordchanged(line, col + count + padcount);
}




/*************************************************
*           Delete chars from line               *
*************************************************/

/* Note that lf_shn is NOT set by this procedure, because it is called
from the screen handler when deletion is reported from the terminal, in
which case a reshow is not wanted. However, the mark may be flagged for
redisplay. */

void line_deletech(linestr *line, int col, int count, BOOL forwardsflag)
{
char *a, *b;
int aftercursor = line->len - col;
int c = aftercursor;
int backcol = col;

if (line->text == NULL) return;    /* Empty line */

a = b = line->text + col;
if (forwardsflag)
  {
  b += count;
  c -= count;
  if (c < 0) count = aftercursor;
  }

else
  {
  a -= count;
  if (c < 0) count += aftercursor;
  backcol -= count;
  if (backcol < 0) backcol = 0;
  }

/* Transfer characters to the undelete queue. We put them in an existing
line if possible; else get a new line and ensure there aren't too many
lines extant. */

if (count > 0)
  {
  char *s = a;

  if (main_undelete == NULL || (main_undelete->flags & lf_udch) == 0 ||
    main_undelete->len + 2*count > 128)
    {
    linestr *new = store_getlbuff((count > 64)? 2*count : 128);
    new->flags |= lf_udch;
    new->len = 0;
    new->next = main_undelete;
    if (main_lastundelete == NULL) main_lastundelete = new;
      else main_undelete->prev = new;
    main_undelete = new;
    main_undeletecount++;
    while (main_undeletecount > max_undelete)
      {
      linestr *prev = main_lastundelete->prev;
      if (prev == NULL) break;   /* Should not occur */
      prev->next = NULL;
      store_free(main_lastundelete->text);
      store_free(main_lastundelete);
      main_lastundelete = prev;
      main_undeletecount--;
      }
    }

  if (forwardsflag)
    {
    int cc;
    char *t = main_undelete->text + main_undelete->len;
    for (cc = 0; cc < count; cc++) { *t++ = TRUE; *t++ = *s++; }
    }
  else
    {
    int cc;
    char *t = main_undelete->text + main_undelete->len + 2*count - 1;
    for (cc = 0; cc < count; cc++) { *t-- = *s++; *t-- = FALSE; }
    }

  main_undelete->len += 2*count;
  }

/* Now close up the line */

while (c-- > 0) *a++ = *b++;

/* If chars were deleted, adjust count, mark line changed,
and sort out the mark position if necessary. */

if (count > 0)
  {
  line->len -= count;
  store_chop(line->text, line->len);
  cmd_recordchanged(line, backcol);

  if (mark_line == line)
    {
    int gap = mark_col - col + (forwardsflag? 0 : count);
    if (gap > 0) mark_col -= (gap > count)? count : gap;
    }
  }
}



/*************************************************
*              Delete line                       *
*************************************************/

linestr *line_delete(linestr *line, BOOL undelete)
{
int i;
linestr *prevline = line->prev;
linestr *nextline = mac_nextline(line);

nextline->prev = prevline;
if (prevline == NULL) main_top = nextline; else prevline->next = nextline;

/* If required, add the line to the deleted list, ensuring that there are
only so many lines on the list. */

if (undelete)
  {
  line->prev = NULL;
  line->next = main_undelete;
  if (main_lastundelete == NULL) main_lastundelete = line;
    else main_undelete->prev = line;
  main_undelete = line;
  main_undeletecount++;
  while (main_undeletecount > max_undelete)
    {
    linestr *prev = main_lastundelete->prev;
    if (prev == NULL) break;   /* Should not occur */
    prev->next = NULL;
    store_free(main_lastundelete->text);
    store_free(main_lastundelete);
    main_lastundelete = prev;
    main_undeletecount--;
    }
  }

/* Otherwise free the line's store. If we are in screen mode and not windowing,
ensure that the table of lines that are currently displayed does not contain
the line we are about to throw away. This is necessary because a new line could
be read before re-display happens, and it might re-use the same block of store,
leading to confusion. We can't set the value to NULL, as that implies the
screen line is blank. Use (+1) and assume that will never be a valid line
address... */

else
  {
  #ifdef Windowing
  if (main_screenOK && !main_windowing)
  #else
  if (main_screenOK)
  #endif
    {
    int i;
    for (i = 0; i <= window_depth; i++)
      if (window_vector[i] == line) window_vector[i] = (linestr *)(+1);
    }

  store_free(line->text);
  store_free(line);
  }

/* Remove deleted lines from the back list */

for (i = 0; i < back_size; i++)
  {
  if ((main_backlist[i]).line == line)
    {
    int j;
    for (j = i + 1; j < back_size; j++) main_backlist[j-1] = main_backlist[j];
    (main_backlist[back_size-1]).line = NULL;
    if (main_backptr > i) main_backptr--;
    }
  }

cmd_recordchanged(nextline, 0);

if (mark_line == line)
  {
  mark_line = nextline;
  mark_col = 0;
  nextline->flags |= lf_shn;
  }

main_linecount--;
return nextline;
}



/*************************************************
*               Align line                       *
*************************************************/

/* The final argument is the address of a second result --
the number of spaces inserted into the line. It may be
negative. */

void line_leftalign(linestr *line, int col, int *a_count)
{
int i;
int leftsig = -1;
int oldlen = line->len;
char *p = line->text;

for (i = 0; i < oldlen; i++) if (p[i] != ' ') { leftsig = i; break; }

if (leftsig == col)
  {
  *a_count = 0;
  return;
  }

if (leftsig < col)
  {
  int insert = col - leftsig;
  *a_count = insert;
  line_insertch(line, 0, NULL, 0, insert);
  }

else
  {
  int extract = leftsig - col;
  *a_count = -extract;
  line_deletech(line, 0, extract, TRUE);
  }

cmd_recordchanged(line, col);
}




/*************************************************
*                 Split Line                     *
*************************************************/

/* The yield is the pointer to the new line, that is,
the second part of the old line. */

linestr *line_split(linestr *line, int col)
{
int newlen = (line->len > col)? line->len - col : 0;
linestr *splitline = store_getlbuff(newlen);
linestr *nextline = line->next;

char *fp = line->text;
char *tp = splitline->text;

splitline->prev = line;
if (nextline == NULL) main_bottom = splitline;
  else nextline->prev = splitline;
splitline->next = nextline;
line->next = splitline;

if (newlen > 0) memcpy(tp, fp+col, newlen);

/* Adjust mark data if mark was in second part of line */

if (mark_line == line && mark_col >= col)
  {
  mark_line = splitline;
  mark_col = mark_col - col;
  }

/* If line was marked for showing, mark second part */

if ((line->flags & lf_shn) != 0) splitline->flags |= lf_shn;

/* If line was marked for clearing at end (usually the
eof marker) mark the second half, and unmark the first
half if the split was in the real text part. */

else if ((line->flags & lf_clend) != 0)
  {
  if (col <= line->len) line->flags &= ~lf_clend;
  splitline->flags |= lf_clend;
  }

/* Adjust lengths of both parts */

if (col < line->len)
  {
  line->len = col;
  store_chop(line->text, col);
  }
splitline->len = newlen;

/* Deal with a split of the end of file line */

if ((line->flags & lf_eof) != 0)
  {
  line->flags &= ~lf_eof;
  line->flags |= lf_clend;
  splitline->flags |= lf_eof;
  if (col != cursor_offset) splitline->flags |= lf_shn;
  }

cmd_recordchanged(splitline, 0);

main_linecount++;
return splitline;
}



/*************************************************
*       Concatenate line with previous           *
*************************************************/

/* The yield is the new line. The existence
of a previous line has already been checked. */

linestr *line_concat(linestr *line, int padcount)
{
linestr *prev = line->prev;
int newlen = line->len + prev->len + padcount;
int backcol = prev->len;
char *newtext = store_Xget(newlen);
char *p;

if (mark_line == line) mark_col += prev->len + padcount;

if (prev->len > 0) memcpy(newtext, prev->text, prev->len);
for (p = newtext + prev->len; p < newtext + prev->len + padcount; p++) *p = ' ';
if (line->len > 0)
  memcpy(newtext + prev->len + padcount, line->text, line->len);

store_free(line->text);
line->text = newtext;
line->len = newlen;
line->key = prev->key;
line->flags |= lf_shn;

(void) line_delete(prev, FALSE);
cmd_recordchanged(line, backcol);
return line;
}



/*************************************************
*                  Verify line                   *
*************************************************/

/* Shouldn't be called for eof line unless shownumber = TRUE */

void line_verify(linestr *line, BOOL shownumber, BOOL showcursor)
{
if (shownumber)
  {
  if (line->key > 0) error_printf("%d.", line->key);
    else error_printf("****.");
  }

if ((line->flags & lf_eof) != 0) error_printf("*\n"); else
  {
  int i;
  BOOL nonprinting = FALSE;
  char *p = line->text;
  if (shownumber) error_printf("\n");

  for (i = 0; i < line->len; i++)
    {
    int c = *p++;
    if (ch_tab2[c] ||
      (main_eightbit && main_interactive && (c & 255) >= topbit_minimum))
        error_printf("%c", c);
    else
      {
      nonprinting = TRUE;
      error_printf("%x", (c >> 4) & 15);
      }
    }
  error_printf("\n");

  if (nonprinting)
    {
    p = line->text;
    for (i = 0; i < line->len; i++)
      {
      int c = *p++;
      if (ch_tab2[c] ||
        (main_eightbit && main_interactive && (c & 255) >= topbit_minimum))
          error_printf(" ");
      else
        error_printf("%x", c & 15);
      }
    error_printf("\n");
    }

  if (showcursor && cursor_col > 0)
    {
    for (i = 1; i < cursor_col; i++) error_printf(" ");
    error_printf(">");
    main_verified_ptr = TRUE;
    }
  error_printflush();
  }
}


/*************************************************
*         Support functions for formatting       *
*************************************************/

/* Functions for determining paragraph beginnings and endings. If the offset
is non-zero, we want to check from there on. */

static BOOL parbegin(linestr *line, int offset)
{
if (par_begin == NULL)
  {
  int i;
  for (i = offset; i < line->len; i++)
    if (line->text[i] != ' ') return TRUE;
  return FALSE;
  }
else
  {
  int USW = 0;
  if (!cmd_casematch) USW |= qsef_U;
  match_leftpos = offset;
  match_rightpos = line->len;
  return cmd_matchse(par_begin, line, USW);
  }
}

/* The offset is added to help with formatting indented and/or tagged 
paragraphs. If it is non-zero, check the remainder of the line against the 
configured ending pattern. */

static BOOL parend(linestr *line, int offset)
{
if (par_end == NULL) return (line->len == 0)? TRUE : line->text[0] == ' '; else
  {
  int USW = 0;
  if ((line->flags & lf_eof) != 0) return TRUE;
  if (!cmd_casematch) USW |= qsef_U;
  match_leftpos = 0;
  match_rightpos = line->len;
   
  /* old code
  return cmd_matchse(par_end, line, USW);
  */
    
  if (cmd_matchse(par_end, line, USW)) return TRUE;
  
  if (offset > 0)
    {
    match_leftpos = offset;
    return cmd_matchse(par_end, line, USW); 
    }  
  return FALSE;   
  }
}

/* Functions for helping decide whether to concatenate lines */

static int spaceleft(linestr *line, int width)
{
int w = line->len;
return (w > 0 && line->text[w-1] != ' ')? width - w - 1 : width - w;
}

static int firstwordlen(linestr *line)
{
int i;
int n = 0;
char *p = line->text;
for (i = 0; i < line->len; i++)
  {
  if (*p++ == ' ') break;
  n++;
  }
return n;
}

/* Procedure to remove trailing spaces */

static void detrail(linestr *line)
{
int i;
char *p = line->text;
for (i = line->len - 1; i >= 0; i--) if (p[i] != ' ') break;
line->len = i + 1;
store_chop(line->text, line->len);
}



/*************************************************
*       Cut out or copy out part of a line       *
*************************************************/

/* The yield is a new line containing the characters that have
been cut out. If cutting, the original line is closed up, but
still in the same buffer. */

linestr *line_cutpart(linestr *line, int left, int right, BOOL copyflag)
{
int i;
int count = right - left;
linestr *cutline = store_getlbuff(count);
char *ip = line->text + left;
char *op = cutline->text;

for (i = left; i < right; i++) *op++ = (i >= line->len)? ' ' : *ip++;

cutline->len = count;
cutline->flags |= lf_shn;

if (!copyflag)
  {
  if (right >= line->len)
    {
    if (left < line->len)
      {
      line->len = left;
      line->flags |= lf_clend;
      cmd_recordchanged(line, left);
      }
    }
  else
    {
    line_deletech(line, left, count, TRUE);
    line->flags |= lf_shn;
    }
  }

return cutline;
}



/*************************************************
*              Format Paragraph                  *
*************************************************/

/* This procedure is called only when there is a non-eof current line. */

void line_formatpara(void)
{
char leftbuf[20];
char *q = leftbuf;
char *p;
int indent = 0;
int indent2 = 0;
int leftbuflen = 0;
int minlen = 0;
int len;
int width = main_rmargin;
BOOL one_line_para;
linestr *nextline = mac_nextline(main_current);

/* If margin disabled, use the remembered value */

if (width > MAX_RMARGIN) width -= MAX_RMARGIN;

/* If we are near the end of the file, we want to be sure that the whole screen
is used. The ScreenHint routine can be coerced into doing the right thing. */

if (main_screenOK) scrn_hint(sh_insert, -1, NULL);

/* If we are not at a beginning-of-paragraph line, just move on. */

if (!parbegin(main_current, 0))
  {
  main_current = nextline;
  main_currentlinenumber++;
  cursor_col = 0;
  return;
  }
  
/* Search the start of the current line for initial indentation and tag text
that might apply to the whole paragraph. But ignore trailing spaces. */

p = main_current->text;

len = main_current->len;
while (len > 0 && p[len-1] == ' ') len--;

while (p - main_current->text < len && *p == ' ')
  {
  indent++;
  p++; 
  } 
  
while (p - main_current->text < len && strchr("#%*+=|~<> ", *p) != NULL)
  {
  *q++ = *p++;
  if (q - leftbuf > 16) { q = leftbuf; break; }
  }  
*q = 0;
leftbuflen = strlen(leftbuf);

/* If we have found any potential indentation and text, use it if either this 
is a one-line paragraph, or if it matches the next line. */

one_line_para = parend(nextline, indent + leftbuflen);
if ((indent > 0 || leftbuflen > 0) && !one_line_para)
  {
  BOOL magic = FALSE;
  p = nextline->text;

  if (indent < nextline->len)
    {
    int i;
    for (i = 0; i < indent; i++) if (*p++ != ' ') break;
    magic = i >= indent;
    }  

  if (magic && leftbuflen > 0)
    {
    if (leftbuflen > nextline->len - indent ||
        strncmp(p, leftbuf, leftbuflen) != 0)
    leftbuflen = 0;      
    } 

  if (! magic) indent = leftbuflen = 0;
  } 
  
/* Total length of indent + starting string */ 
  
minlen = indent + leftbuflen;

/* Now that we know the indent and text, we must re-check that this line
is a paragraph beginner, taking the preliminary stuff into account. */

if (indent > 0 || leftbuflen > 0)
  {
  if (!parbegin(main_current, minlen))
    {
    main_current = nextline;
    main_currentlinenumber++;
    cursor_col = 0;
    return;
    }
  } 

/* We also want to cope with the case where the second and subsequent lines
of the paragraph are indented more than the first one, after taking the common 
indent and start string into account. */

if (!one_line_para)
  {
  p = nextline->text + minlen;
  while (p - nextline->text < nextline->len && *p == ' ') 
    {
    indent2++;
    p++;
    }  
  } 

/* Start of main loop - exit by return */

for (;;)
  {
  detrail(main_current);

  /* Loop until current line is short enough */

  while (main_current->len > width)
    {
    BOOL ended; 
    int i, j;
    char *p = main_current->text;

    cmd_recordchanged(main_current, 0);

    for (i = width; i >= minlen; i--) if (p[i] == ' ') break;
    if (i < minlen) i = j = width; else
      {
      j = i + 1;
      while (j < main_current->len)
        if (p[j] == ' ') j++; else break;
      while (i > minlen && p[i-1] == ' ') i--;
      }

    /* If next is paragraph end, make a new part line and make it the current
    line. The paragraph ending check is repeated later, but I haven't got round
    to tidying this into a subroutine. */

    ended = parend(nextline, minlen);
    
    /* In addition to the normal check, we must check for matching indents and 
    tag. */

    if (!ended && minlen > 0)
      {   
      char *p = nextline->text;

      if (indent < nextline->len)
        {
        int i;
        for (i = 0; i < indent; i++) if (*p++ != ' ') break;
        ended = i < indent;
        }  
      
      if (!ended && leftbuflen > 0)
        ended = leftbuflen > nextline->len - indent ||
            strncmp(p, leftbuf, leftbuflen) != 0;
            
      if (!ended && indent2 > 0)
        {
        if (nextline->len < indent + leftbuflen + indent2) ended = TRUE; else
          { 
          int i;
          p += leftbuflen;
          for (i = 0; i < indent2; i++) if (*p++ != ' ') break;
          ended = i < indent2; 
          } 
        }  
      }
 
    /* The next line is not to be included in this paragraph. So what we split 
    off this line must become a new line on its own. */
     
    if (ended)
      {
      linestr *extra = line_cutpart(main_current, j, main_current->len, FALSE);
      main_current->len -= j - i;
      if (main_current->len == width) main_current->flags |= lf_shn;
      main_current->next = extra;
      extra->next = nextline;
      nextline->prev = extra;
      extra->prev = main_current;
      main_current = extra;
      main_linecount++;
      main_currentlinenumber++;

      /* If there's an indent or a tag or 2nd line indent, insert them. */
       
      if (indent > 0) line_insertch(main_current, 0, NULL, 0, indent);
      if (leftbuflen > 0) 
        line_insertch(main_current, indent, leftbuf, leftbuflen, 0);
      if (indent2 > 0)
        line_insertch(main_current, minlen, NULL, 0, indent2);   
      }

    /* Otherwise, move text onto the start of the next line, and make it the
    current line. We know that if there's an indent or tag, or second indent,
    they will exist on the next line, so just insert after them. */

    else
      {
      line_insertch(nextline, indent+leftbuflen+indent2, p + j, 
        main_current->len - j, 1);
      nextline->flags |= lf_shn;
      main_current->len = i;                   /* shorten current */
      if (main_current->len == width) main_current->flags |= lf_shn;
        else main_current->flags |= lf_clend;  /* clear out end */
      main_current = nextline;
      main_currentlinenumber++;
      nextline = mac_nextline(nextline);
      }

    detrail(main_current);                     /* includes store_chop() */
    }

  /* We now have a current line that is shorter than the required width.
  Concatenate with following lines until it exceeds the width, but only if
  there is room for the next word on the line. However, if the end of the
  paragraph is reached, exit from the procedure. */

  while (main_current->len <= width)
    {
    BOOL ended = parend(nextline, minlen);
    
    /* This code is repeated from above. It should be tidied up into a function 
    one day. Possibly it should all be included in parend(). */ 
    
    if (!ended && (indent > 0 || leftbuflen > 0))
      {   
      char *p = nextline->text;

      if (indent < nextline->len)
        {
        int i;
        for (i = 0; i < indent; i++) if (*p++ != ' ') break;
        ended = i < indent;
        }  
      
      if (!ended && leftbuflen > 0)
        ended = leftbuflen > nextline->len - indent ||
            strncmp(p, leftbuf, leftbuflen) != 0;

      if (!ended && indent2 > 0)
        {
        if (nextline->len < indent + leftbuflen + indent2) ended = TRUE; else
          { 
          int i;
          p += leftbuflen;
          for (i = 0; i < indent2; i++) if (*p++ != ' ') break;
          ended = i < indent2; 
          } 
        }  
      }
      
    /* The next line is not part of this paragraph */
 
    if (ended)
      {
      main_current = nextline;
      main_currentlinenumber++;
      cursor_col = indent;
      return;
      }

    /* We try to decide whether it is worth joining the lines. This is an
    optimisation that works in common cases, but not when the second line
    starts with a space. However, it is fail-safe. */

    if (firstwordlen(nextline) <= spaceleft(main_current, width))
      {
      int sp = 0;

      cmd_recordchanged(main_current, 0);
      
      /* If an indent or tag exists, it has been checked to be on the next 
      line, so we can remove them. */

      if (indent > 0 || leftbuflen > 0)
         line_deletech(nextline, 0, indent+leftbuflen+indent2, TRUE);

      if (main_current->len > 0 &&
        main_current->text[main_current->len - 1] != ' ' &&
          nextline->len > 0 && nextline->text[0] != ' ')
            sp = 1;

      main_current = line_concat(nextline, sp);
      }

    else
      {
      main_current = nextline;
      main_currentlinenumber++;
      }

    detrail(main_current);
    nextline = mac_nextline(main_current);
    }
  }
}

/* End of eline.c */
