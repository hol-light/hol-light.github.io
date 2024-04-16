/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: September 1994 */


/* This file contains code for matching a search expression */


#include "ehdr.h"



/*************************************************
*            Check 'word'                        *
*************************************************/

static BOOL chkword(int p, int len, char *t, int wleft, int wright)
{
int n = p + len;
if (p > wleft && (ch_tab[(unsigned char)(t[p-1])] & ch_word) != 0) return FALSE;
if (n < wright && (ch_tab[(unsigned char)(t[n])] & ch_word) != 0) return FALSE;
return TRUE;
}



/*************************************************
*         Match char string to line string       *
*************************************************/

static BOOL matchchars(char *s, int len, char *t, BOOL U)
{
int i;
if (U)
  {
  for (i = 0; i < len; i++) if (toupper(s[i]) != toupper(t[i])) return FALSE;
  }
else
  {
  for (i = 0; i < len; i++) if (s[i] != t[i]) return FALSE;
  }

return TRUE;
}



/*************************************************
*        Match qualified string to line          *
*************************************************/

static BOOL matchqs(qsstr *qs, linestr *line, int USW)
{
char *t = line->text;
char *s = qs->text + 1;                               /* first char is delimiter */

int p;                                                /* match point */
int count = qs->count;
int flags = qs->flags;
int len = qs->length;
int leftpos = match_leftpos;                          /* lhs in line */
int rightpos = match_rightpos;                        /* rhs in line */
int wleft = qs->windowleft;                           /* lhs window */
int wright = qs->windowright;                         /* rhs window */

unsigned int *map = qs->map;

BOOL U = (qs->flags & qsef_U) != 0 ||
  ((USW & qsef_U) != 0 && (qs->flags & qsef_V) == 0); /* upper-case state */
BOOL W = ((qs->flags | USW) & qsef_W) != 0;           /* wordsearch state */
BOOL yield = FALSE;                                   /* default reply */

/* If hex string, adjust pointers */

if ((flags & qsef_X) != 0)
  {
  s = qs->hexed;
  len /= 2;
  }

/* Take note of line length & significant space qualifier */

if (wright > line->len) wright = line->len;
if (((flags | USW) & qsef_S) != 0)
  {
  while (wleft < wright && t[wleft] == ' ') wleft++;
  while (wleft < wright && t[wright-1] == ' ') wright--;
  }

/* Take note of window qualifier */

if (leftpos < wleft) leftpos = wleft;
if (rightpos > wright) rightpos = wright;
if (rightpos < leftpos) rightpos = leftpos;

/* In most cases, search starts at left */

p = leftpos;

/* First check line long enough, then match according to the flags */

if ((leftpos + len <= rightpos))
  {

  /* Deal with B & P (P = B + E); if H is on it must be PH as
  H is not allowed with either B or E on its own. */

  if ((flags & qsef_B) != 0)
    {
    if (p == wleft || (flags & qsef_H) != 0)    /* must be at line start or H */
      {
      /* Deal with P */

      if ((flags & qsef_E) != 0)
        {
        if (len == rightpos - p && matchchars(s, len, t+p, U)) yield = TRUE;
        }

      /* Deal with B on its own */

      else if (matchchars(s, len, t+p, U) &&
        (!W || chkword(p, len, t, p, wright))) yield = TRUE;
      }
    }

  /* Deal with E on its own */

  else if ((qs ->flags & qsef_E) != 0)
    {
    if (rightpos == wright)   /* must be at line end */
      {
      p = rightpos - len;
      if (matchchars(s, len, t+p, U) &&
        (!W || chkword(p, len, t, wleft, wright))) yield = TRUE;
      }
    }

  /* Deal with H */

  else if ((qs-> flags & qsef_H) != 0)
    {
    if (matchchars(s, len, t+p, U) &&
      (!W || chkword(p, len, t, wleft, wright))) yield = TRUE;
    }

  /* Deal with L; if the line character at the first position is not
  in the string, skip backwards the whole length. */

  else if (match_L || (flags & qsef_L) != 0)
    {
    p = rightpos - len;
    
    if (len == 0) yield = TRUE; else while (p >= leftpos)
      {
      int c = (unsigned char)(t[p]);
      if ((map[c/intbits] & (1 << (c%intbits))) == 0) p -= len; else
        {
        if (matchchars(s, len, t+p, U) &&
          (!W || chkword(p, len, t, wleft, wright)))
            {
            if (--count <= 0)  { yield = TRUE; break; }
            p -= len;
            }
        else p--;
        } 
      }
    }

  /* else it's a straight forward search; if the line character at the
  last position is not in the string, skip forward by the whole length. */

  else 
    {
    if (len == 0) yield = TRUE; else while (p + len <= rightpos)
      {
      int c = (unsigned char)(t[p+len-1]);
      if ((map[c/intbits] & (1 << (c%intbits))) == 0) p += len; else
        { 
        if (matchchars(s, len, t+p, U) &&
          (!W || chkword(p, len, t, wleft, wright)))
            {
            if (--count <= 0) { yield = TRUE; break; }
            p += len;
            }
        else p++;
        } 
      }
    } 

  /* If successful non-C match, set start & end */

  if (yield)
    {
    match_start = p;
    match_end = p + len;
    }
  }

/* If N is present, reverse result */

if ((flags & qsef_N) != 0)
  {
  yield = !yield;
  if (yield)
    {
    match_start = 0;    /* give whole line */
    match_end = line->len;
    }
  }

return yield;
}



/*************************************************
*        Match a search expression to a line     *
*************************************************/

/* The yield is TRUE or FALSE; if TRUE, the global
variables match_start and match_end contain the
start and end of the matched string (or whole line).
The left and right margins for the search are set
in globals to avoid too many arguments. There is
also the flag match_L, set during backwards
find operations. */

BOOL cmd_matchse(sestr *se, linestr *line, int USW)
{
/* Pass down U, S and W flags -- V turns U off */

if ((se->flags & qsef_U) != 0) USW |= qsef_U;
if ((se->flags & qsef_V) != 0) USW &= ~qsef_U;
if ((se->flags & qsef_S) != 0) USW |= qsef_S;
if ((se->flags & qsef_W) != 0) USW |= qsef_W;

/* Test for qualified string */

if (se->type == cb_qstype)
  return ((se->flags & qsef_R) != 0)?
    cmd_matchqsR((qsstr *)se, line, USW) : matchqs((qsstr *)se, line, USW);

/* Got a search expression node - yield is a line */

else
  {
  BOOL yield = cmd_matchse(se->left.se, line, USW);

  /* Deal with OR */
  
  if ((se->flags & qsef_AND) == 0)
    {
    if (!yield && se->right.se != NULL)
      yield = cmd_matchse(se->right.se, line, USW);
    }

  /* Deal with AND */

  else if (yield) yield = cmd_matchse(se->right.se, line, USW);

  /* Invert yield if N set */

  if ((se->flags & qsef_N) != 0) yield = !yield;

  /* If matched, return whole line */

  if (yield)
    {
    match_start = 0;
    match_end = line->len;
    }

  return yield;
  }
}

/* End of ematch.c */
