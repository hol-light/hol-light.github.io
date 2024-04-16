/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 2001 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: June 2001 */


/* This file contains code for matching a regular expression */


#include "ehdr.h"
#include "rehdr.h"


static BOOL Journal_Overflowed;


/*************************************************
*   Match Regular Expression: Make state active  *
*************************************************/

/* If the state is already active, the R_Listed bit
in R_Sflags will be set. If the R_Endalt bit is set
for the new state, add a dummy entry to the journal
to terminate the wild string. */

static void MakeActiveState(int n, int ch, int old, int type)
{
if (n > (int)R_String[0]) n = 0;    /* past end = "done" */

/* Maintain Journal if required */

if (R_Journal != NULL && type >= 0 && R_Jptr < JournalSize)
  {
  R_Journal[R_Jptr] = old;
  R_Journal[R_Jptr+1] = ch;
  R_Journal[R_Jptr+2] = n;
  R_Journal[R_Jptr+3] = type;
  R_Jptr += 4;
  }

if ((R_Sflags[n] & R_Listed) == 0)
  {
  R_Sflags[n] |= R_Listed;
  R_List[R_ListPtr++] = n;

  if ((R_Sflags[n] & R_Endalt) != 0 && R_Journal != NULL && R_Jptr < JournalSize)
    {
    R_Journal[R_Jptr] = n;
    R_Journal[R_Jptr+1] = 0;
    R_Journal[R_Jptr+2] = n;
    R_Journal[R_Jptr+3] = 6;
    R_Jptr += 4;
    }
  }
}


/*************************************************
*            Match Regular Expression            *
*************************************************/

/* We aim to find the longest possible match. The search
ends if

(a) No chars are left to match -- failure;
(b) End of line reached and success state is active or was
    active at some point -- success;
(c) Success state is the only active state.

Journalling is used to record what actually got matched, in order to
pick up the wild strings. This takes store, so is only done when the
feature is requested. The global R_Journal must be set by the caller
to point to a vector whose length is the manifest constant JournalSize.
If R_Journal is zero, no journalling is performed. The entries in the
journal record each change of state of the finite state machine, and
they occupy four bytes each:

byte 0    current finite-state machine state number
byte 2    character matched, or zero if change is not caused
          by a character match
byte 3    new state that has become active
byte 4    reason coding:
  0   automatic next after non-matching state -- this indicates
      the start of a wild sequence
  1   automatic successor to a # or ^ state
  2   other explicit successor (e_g. alternative)
  3   single character wild match
  4   single character non-wild match
  5   start of bracketed sequence
  6   end of top-level bracketed sequence

We start building the journal at word 1 (byte offset 4) so as to
leave enough space to build the wild string list in place in it
afterwards.

The journal is scanned backwards at the end, to remove all entries
except those which led to the final matching state. The wild strings
are then EITHER (a) any characters between a change of type 0 and a
change of type 1 OR (b) individual characters of type 3. They are
placed in the journal vector as a sequence of BCPL strings. */

BOOL cmd_matchqsR(qsstr *qs, linestr *line, int USW)
{
char *chars = line->text;

int count = qs->count;
int flags = qs->flags;
int len = qs->rlength;
int leftpos = match_leftpos;
int rightpos = match_rightpos;
int wleft = qs->windowleft;
int wright = qs->windowright;
int p;                    /* current posn */

BOOL U = (flags & qsef_U) != 0 || ((USW & qsef_U) != 0 && (flags & qsef_V) == 0);
BOOL W = (flags & qsef_W) != 0 || (USW & qsef_W) != 0;
BOOL yield = FALSE;
BOOL backwards = (flags & qsef_L) != 0 || ((match_L || (flags & qsef_E) != 0) &&
  (flags & qsef_B) == 0);

Journal_Overflowed = FALSE;

/* Take note of line length && sig space qualifier */

if (wright > line->len) wright = line->len;
if ((flags & qsef_S) != 0 || (USW & qsef_S) != 0)
  {
  while (wleft < wright && chars[wleft] == ' ') wleft++;
  while (wleft < wright && chars[wright-1] == ' ') wright--;
  }

/* Take note of window qualifier */

if (leftpos < wleft) leftpos = wleft;
if (rightpos > wright) rightpos = wright;
if (rightpos < leftpos) rightpos = leftpos;

/* If there's no finite state machine data, or if it is compiled
in the wrong direction (which can happen if the argument of an
F command is reused with a BF command or vice versa), then
compile it. */

if (qs->fsm == NULL ||
  (backwards && (flags & qsef_REV) == 0) ||
    (!backwards && (flags & qsef_REV) != 0))
       cmd_makeFSM(qs);

/* Set up global pointers to the FSM vectors */

R_String = qs->fsm;
R_States = R_String + len + 1;
R_Sflags = R_States + len + 1;
R_List   = R_Sflags + len + 1;

/* Set starting position in the line, less one, remembering
it in case of a match. */

if (backwards)
  {
  p = rightpos;
  match_end = p;
  }
else
  {
  p = leftpos - 1;
  match_start = leftpos;
  }

/* Start of Outer Loop for moving along the line, looking for a match.
The variable LASTMATCH keeps the p (i_e. the last char of the match)
for the most recent successful position, for back-tracking if we run
out of chars. Since the p pointer starts one char before the first
matchable char, it can legally take on a value of -1, and so therefore
can LASTMATCH if null is a valid alternative for the string. Hence the
use of a value less than -1 to mean "unset". */

if (((flags & qsef_B) != 0 && p+1 != wleft) ||
  ((flags & qsef_B) == 0 && (flags & qsef_E) != 0 && p != wright))
    yield = FALSE;

else for (;;)
  {
  BOOL matched = FALSE;
  int lastmatch = -2;                 /* for saving match positions */
  R_ListPtr = 0;                      /* for list of active states */
  R_Jptr = 4;                         /* journal pointer */

  /* Make the initial state active. If it is a repeat state, we force
  a journal entry indicating "start of wild string". This is necessary
  for a ^ repeat, since it otherwise starts at a normal character; it
  is also necessary for a # repeat, in case the wild string is null,
  in which case the normal start mark will get deleted because it
  is not on the final match path. */

  if (R_String[1] == '^') MakeActiveState(2, 0, 1, 0);
    else if (R_String[1] == '#') MakeActiveState(1, 0, 1, 0);
      else MakeActiveState(1, 0, 0, -1);   /* no journalling */

  /* Debugging output */

  #ifdef REDEBUG
  debug_printf("Start outer loop with p = %d\n", p);
  #endif

  /* If there is an outer alternation, make its states active
  without journalling. */

  if (R_States[0] != 0) MakeActiveState(R_States[0], 0, 0, -1);

  /* Start of Inner Loop, which is the execution loop of
  the finite state machine. */

  for (;;)
    {
    int i, ch, realch, oldtop;
    int clptr = 0;

    /* First we complete the closure: scan all active states
    and, for non-match states, activate the next state in
    sequence as well as any explicit successor state.
    When the next character is '^' ("one or more") the next
    state in sequence is one greater than usual. Note
    that R_ListPtr will be increasing during this process, and
    we want to include newly added states in the closure process.
    MakeActiveState checks for states that are already active,
    and does nothing, so there is no danger of looping for
    ever. It also maintains the journal when required. */

    while (clptr < R_ListPtr)
      {
      int n = R_List[clptr];
      if (n != 0 && (R_Sflags[n] & R_MatchState) == 0)
        {
        int ch = R_String[n];
        int nextch = (n == R_String[0])? 0 : R_String[n+1];
        int nextstate = (nextch == '^')? n+2 : n+1;
        if (nextstate > (int)R_String[0]) nextstate = 0;  /* end => done */

        /* If the state we are looking at is start of bracket, make
        a dummy journal entry for identification. */

        if (ch == '(') MakeActiveState(n,0,n,5);

        /* Now activate the successor state */

        MakeActiveState(nextstate,0,n,0);

        /* A repeat state always has an explicit successor,
        which may be zero (success). Other non-match states
        have only non-zero successors. */

        nextstate = R_States[n];

        if (ch == '#' || ch == '^' || nextstate != 0)
          {
          MakeActiveState(nextstate, 0, n, (ch=='#' || ch=='^')? 1:2);

          /* If the successor state is one past '^' we must make a
          dummy entry to indicate the start of a wild string. This is
          not necessary for '#' because it causes another successor
          state to be activated. */

          if (R_String[nextstate-1] == '^')
            MakeActiveState(nextstate, 0, nextstate, 0);
          }
        }
      clptr++;
      }

    /* Debugging output */

    #ifdef REDEBUG
    debug_printf("Active: ");
    for (i = 0; i < R_ListPtr; i++) debug_printf("%3d", R_List[i]);
    debug_printf("\n");
    #endif

    /* Scan active states, removing 'on list' bit for
    future use. Check if state 0 active, and if so,
    remember this position. */

    for (i = 0; i < R_ListPtr; i++)
      {
      int n = R_List[i];
      if (n == 0)
        {
        lastmatch = p;
        }
      R_Sflags[n] &= R_XListed;
      }

    /* If the only active state is state 0, then we have a
    match; else if there are no active states then failure
    unless there was a previous match point; else do a char
    match on the line. */

    if (R_ListPtr == 1 && R_List[0] == 0)
      {
      matched = TRUE;
      break;                       /* to end inner loop */
      }

    if (R_ListPtr == 0)
      {
      if (lastmatch >= -1)
        {
        p = lastmatch;            /* re-position */
        matched = TRUE;
        }
      break;                      /* to end inner loop */
      }

    /* Now we advance to the next char in the line; if
    there are no more then we have failed unless there
    was a previous match. */

    if (backwards) p--; else p++;
    if (p < leftpos || p >= rightpos)
      {
      if (lastmatch >= -1)
        {
        p = lastmatch;       /* re-position */
        matched = TRUE;
        }
      break;                 /* to end inner loop */
      }

    realch =  ch = chars[p];
    if (U) ch = toupper(ch);

    /* Reset the state list pointer for building a new list, and
    scan up the old list of states. Each state can add at most
    one successor state, so there will be no overlap of store. */

    oldtop = R_ListPtr;
    R_ListPtr = 0;

    for (i = 0; i < oldtop; i++)
      {
      int n = R_List[i];       /* state number */
      int successptr = n;     /* where successor is to be found */
      int waswild = 3;

      /* We are only interested in char matching states */

      if ((R_Sflags[n] & R_MatchState) != 0)
        {
        int chmatch = FALSE;
        int j = successptr;   /* char pointer */
        int kch = R_String[j];
        if (kch == '~' || kch == '\"') successptr++;

        /* Debugging output */

        #ifdef REDEBUG
        {
        int jj = j + 1;
        int dd = kch;
        debug_printf("State %d line=%c string=%c", n, ch, dd);
        if (dd == '~') { dd = R_String[jj++]; debug_printf("%c",dd); }
        if (dd == '$' || dd == '\"') debug_printf("%c", R_String[jj]);
        debug_printf("\n");
        }
        #endif

        /* The wild character matches anything */

        if (kch == '?') chmatch = TRUE; else
          {
          int negalt = FALSE;

          /* The not character reverses the sense */

          if (kch == '~')
            {
            chmatch = TRUE;
            kch = R_String[++j]; /* skip to next char */

            if (kch == '(')      /* 'negative alternation' */
              {
              negalt = TRUE;
              kch = R_String[++j];
              }
            }

          /* Start of a loop which is repeated only for 'negative
          alternatives'. It checks against a single line character. */

          for (;;)
            {
            int onehex = FALSE;       /* indicates individual hex */
            int dollargroup = FALSE;  /* indicates $<letter> processed */

            /* Deal with $ special codes; the 'h' code sets
            the onehex flag to make use of the hex code handling
            later; otherwise dollargroup indicates that the
            matching check has been done. */

            if (kch == '$')
              {
              int c = R_String[++j];
              dollargroup = TRUE;
              if (U) c = toupper(c);
              switch (c)
                {
                case 'h': case 'H':
                onehex = TRUE;
                j++;
                break;

                case 'A':
                if (isupper(ch) || isdigit(ch)) chmatch = !chmatch;
                break;

                case 'a':
                if (islower(ch) || isdigit(ch)) chmatch = !chmatch;
                break;

                case 'd': case 'D':
                if (isdigit(ch)) chmatch = !chmatch;
                break;

                case 'L':
                if (isupper(ch)) chmatch = !chmatch;
                break;

                case 'l':
                if (islower(ch)) chmatch = !chmatch;
                break;

                case 'P': case 'p':
                if (!isalnum(ch)) chmatch = !chmatch;
                break;

                case 'S': case 's':
                if (ch == ' ' || ch == '\t') chmatch = !chmatch;
                break;

                case 'W': case 'w':
                if ((ch_tab[realch] & ch_word) != 0) chmatch = !chmatch;
                break;
                }
              }

            /* The quote character looks at the next */

            else if (kch == '\"') j++;

            /* Now deal with a char or a range of chars */

            if ((flags & qsef_X) != 0 || onehex)
              {
              /* The hex digits are in the string as characters,
              but they have been checked for validity. */

              int a = mac_hex(R_String[j], R_String[j+1]);
              int k = (j++) + 2;
              if (k <= (int)R_String[0] && R_String[k] == '-')
                {
                int b;
                j = k + 2;
                if (R_String[j-1] == '\"') j++;
                b = mac_hex(R_String[j-1], R_String[j]);
                if (a <= chars[p] && chars[p] <= b) chmatch = !chmatch;
                }
              else if (chars[p] == a) chmatch = !chmatch;
              }

            /* Not a hexadecimal string */

            else if (!dollargroup)
              {
              if (j < (int)R_String[0] && R_String[j+1] == '-')
                {
                int a = R_String[j];
                int b = R_String[j+2];
                j += 2;
                if (b == '\"') b = R_String[++j];
                if (U)
                  {
                  a = toupper(a);
                  b = toupper(b);
                  }
                if ((a <= ch && ch <= b) && ((ch_tab[a] & ch_tab[ch]) != 0))
                  chmatch = !chmatch;
                }
              else
                {
                int a = R_String[j];
                if (U) a = toupper(a);
                if (ch == a)
                  {
                  chmatch = !chmatch;
                  waswild = 4;
                  }
                }
              }

            /* Deal with negative alternation. If we have matched
            a char, the alternation fails. (This is indicated by chmatch
            being FALSE, since it starts TRUE after a negation.)
            Otherwise, check for more alternatives. Note the syntax
            has been checked on input. */

            if (!negalt) break;  /* from negalt loop */
            if (chmatch)         /* => not matched (!) */
              {
              if (R_String[++j] == ')') break;   /* done, nothing matched, success */
              kch = R_String[++j];               /* next alternative */
              }
            else break;    /* something matched, overall failure */
            }              /* end of negative alternative loop */
          }                /* end of single char match operation */

        /* If we have matched, make successor state active, updating
        the journal as necessary to keep track of what is going on. */

        if (chmatch)
          {
          int nextstate = R_States[successptr];
          MakeActiveState(nextstate,chars[p],R_List[i],waswild);

          /* If successor is '#' or one past '^', add a "start of
          wild" entry to the journal. */

          if (R_String[nextstate-1] == '^' || R_String[nextstate] == '#')
             MakeActiveState(nextstate,0,nextstate,0);
          }
        }
      }    /* Loop for all active states */
    }


  /* We have now completed a single matching operation on the
  line. If this has succeeded then we check additional
  criteria such as B, E, W and the count qualifier. But first
  sort out the wild strings if necessary. */

  if (matched)
    {
    int k;

    /* A non-zero value for R_Journal implies wild string tracing */

    if (R_Journal != NULL)
      {
      int this;           /* pointer variable */
      int woffset = 0;    /* offset for next wild string */

      #ifdef REDEBUGWF    /* write out full journal */
        {
        int xct = 9999;
        for (k = 4; k <= R_Jptr-4; k += 4)
          {
          int cc = R_Journal[k+1];
          int ww = R_Journal[k+3];
          if (++xct > 5) { debug_printf("\n"); xct = 1; }
          if (cc == 0)
            debug_printf("(%d,,%d,%d) ",R_Journal[k],R_Journal[k+2],ww);
          else
            debug_printf("(%d,%c,%d,%d) ",R_Journal[k],cc,R_Journal[k+2],ww);
          }
        debug_printf("\n");
        }
      #endif

      /* Reverse scan of journal to establish what was matched and
      cancel (by setting first byte to zero) all other entries. */

      this = 0;
      for (k = R_Jptr-4; k >= 4; k -= 4)
        if (R_Journal[k+2] == this) this = R_Journal[k];
          else R_Journal[k] = 0;

      /* Debugging output for edited journal */

      #ifdef REDEBUGW
        {
        int xct = 9999;
        for (k = 4; k < R_Jptr; k += 4) if (R_Journal[k])
          {
          int cc = R_Journal[k+1];
          int ww = R_Journal[k+3];
          if (++xct > 5) { debug_printf("\n"); xct = 1; }
          if (cc == 0)
            debug_printf("(%d,,%d,%d) ",R_Journal[k],R_Journal[k+2],ww);
          else
            debug_printf("(%d,%c,%d,%d) ",R_Journal[k],cc,R_Journal[k+2],ww);
          }
        debug_printf("\n");
        }
      #endif

      /* Now establish the wild strings; the start of a bracketed
      group supersedes any other form of wild string. Note the
      change of endtype if a nested bracketed group is found. This
      deals with the case #(..). */

      R_Journal[woffset] = 255;   /* default no wild strings */
      this = 4;                   /* scan start point */

      while (this < R_Jptr)
        {
        while (this < R_Jptr && R_Journal[this] == 0) this += 4;
        if (this >= R_Jptr) break;
        switch (R_Journal[this+3])
          {
          case 0:    /* start of wild string */
          case 5:    /* start of bracketed group */
            {
            int n = 0;        /* count */
            int endtype = (R_Journal[this+3] == 0)? 1 : 6;
            this += 4;
            while (this < R_Jptr)
              {
              if (R_Journal[this] > 0)
                {
                int type = R_Journal[this+3];
                if (type == 5) endtype = 6;
                else if (type == endtype) break;
                else if (type == 3 || type == 4)
                  {
                  R_Journal[woffset + (++n)] = R_Journal[this+1];
                  }
                }
              this += 4;
              }
            R_Journal[woffset] = n;
            woffset += n + 1;
            R_Journal[woffset] = 255;
            }
          break;

          case 3:    /* single char wild string */
          R_Journal[woffset] = 1;
          R_Journal[woffset+1] = R_Journal[this+1];
          woffset += 2;
          R_Journal[woffset] = 255;
          break;
          }
        this += 4;
        }

      /* If the matching has taken place by scanning the line from
      right to left, we must reverse each of the wild strings, since
      they have been stored from left to right. We must also reverse
      the order of the strings themselves. We reverse the entire
      vector of strings, then move the lengths to the beginning. */

      if (backwards)
        {
        int x = 0;
        int y = woffset - 1;
        while (x < y)
          {
          int t = R_Journal[x];
          R_Journal[x++] = R_Journal[y];
          R_Journal[y--] = t;
          }
        x = woffset - 1;
        while (x >= 0)
          {
          int ii;
          int n = R_Journal[x];
          for (ii = x; ii > x - n; ii--) R_Journal[ii] = R_Journal[ii-1];
          R_Journal[x-n] = n;
          x -= n + 1;
          }
        }

      #ifdef REDEBUGW
      woffset = 0;
      if (R_Journal[woffset] == 255) debug_printf("No wild strings");
      else while (R_Journal[woffset] != 255)
        {
        int ii;
        int n = R_Journal[woffset];
        debug_printf("\"");
        for (ii = woffset+1; ii <= woffset+n; ii++) debug_printf("%c", R_Journal[ii]);
        debug_printf("\" ");
        woffset += n + 1;
        }
      debug_printf("\n");
      #endif
      }

    /* End of Journal handling code */

    #ifdef REDEBUG
    debug_printf("Regular expression matched with end = %d\n", p);
    #endif

    /* Additional check the P qualifier */

    if ((flags & qsef_EB) == qsef_EB)
      {
      if (p != rightpos-1) matched = FALSE;
      }

    /* Additional check for W qualifier */

    if (matched)
      {
      if (W)
        {
        int start, end;
        if (backwards)
          {
          start = p;
          end = match_end;
          }
        else
          {
          start = match_start;
          end = p+1;
          }

        if ((start != wleft && (ch_tab[(unsigned char)(chars[start-1])] &
             ch_word) != 0) ||
          (end != wright && (ch_tab[(unsigned char)(chars[end])] & ch_word)
              != 0))
            matched = FALSE;
        }
      }

    /* Additional check for the count qualifier */

    if (matched)
      {
      if (--count <= 0)
        {
        yield = TRUE;
        if (backwards) match_start = p;
          else match_end = p+1;
        break;                /* to end Outer Loop */
        }

      /* skip over this string */

      if (backwards) match_end = p + 1;
        else match_start = p;
      }
    }

  /* We have failed at this point in the line to find a match.
  Advance to the next character in the line, to try again,
  unless there are no more chars or the B or E qualifier
  is set. */

  if (backwards)
    {
    if (--match_end < leftpos) break;
    p = match_end;
    }
  else
    {
    if (++match_start >= rightpos) break;
    p = match_start - 1;
    }

  if ((flags & (qsef_EB | qsef_H)) != 0) break;
  }

/* The match has either succeeded or failed. If the N qualifier
is present, reverse the yield and set the pointers to include
the whole line if necessary. */

if ((flags & qsef_N) != 0)
  {
  yield = !yield;
  if (yield)
    {
    match_start = 0;
    match_end = line->len;
    }
  }

/* If the match has succeeded but the journal has overflowed,
indicate this by setting a flag. */

if (yield && R_Journal != NULL && R_Jptr >= JournalSize)
  Journal_Overflowed = TRUE;

/* At last, the end of the business */

return yield;
}



/*************************************************
*         Increase buffer size                   *
*************************************************/

/* This is an auxiliary function for cmd_ReChange(). */

static char *incbuffer(char *v, int *sizep)
{
char *newv = store_Xget(*sizep + 1024);
memcpy(newv, v, *sizep);
*sizep += 1024;
return newv;
}


/*************************************************
*    Change Line after Regular Expression match  *
*************************************************/

/* This procedure is called to make changes to a line after it has been
matched with a regular expression. The replacement string is searched
for occurences of the '%' character, which are interpreted as follows:

%0           insert the entire matched string from the original line
%<digit>     insert the <digit>th "wild string" from the match
%<non-digit> insert <non-digit>

If, for example, %6 is encountered and there were fewer than 6 wild
strings found, nothing is inserted. The final argument is the address
of a flag, which is set FALSE if wild strings are required and the
first word of the Journal is -1, indicating that it overflowed. */

linestr *cmd_ReChange(linestr *line, char *p, int len,
  BOOL hexflag, BOOL eflag, BOOL aflag, BOOL *aok)
{
int i, pp;
int n = 0;
int size = 1024;
char *v = store_Xget(size);

*aok = TRUE;

/* Loop to scan replacement string */

for (pp = 0; pp < len; pp++)
  {
  int cc = p[pp];

  /* Ensure at least one byte left in v */

  if (n >= size) v = incbuffer(v, &size);

  /* Deal with non-meta character (not '%') */

  if (cc != '%')
    {
    /* Deal with hexadecimal data */
    if (hexflag)
      {
      int x = 0;
      for (i = 1; i <= 2; i++)
        {
        x = x << 4;
        cc = tolower(cc);
        x += ('a' <= cc && cc <= 'f')? cc - 'a' + 10 : cc - '0';
        cc = p[pp+1];
        }
      pp++;
      v[n++] = x;
      }

    /* Deal with normal data */
    else v[n++] = cc;
    }

  /* Deal with meta-character ('%') */
  else
    {
    if (++pp < len)
      {
      cc = p[pp];
      if (!isdigit(cc)) v[n++] = cc;
      else if (cc == '0')
        {
        char *ppp = line->text;
        while (n + match_end - match_start >= size) v = incbuffer(v, &size);
        for (i = match_start; i < match_end; i++) v[n++] = ppp[i];
        }
      else if (R_Journal != NULL)
        {
        int c = cc - '0' - 1;
        int j = 0;
        if (Journal_Overflowed) { *aok = FALSE; return line; }

        while (c-- != 0 && R_Journal[j] != 255) j += R_Journal[j] + 1;

        if (R_Journal[j] != 255)
          {
          while (n + (int)R_Journal[j] >= size) v = incbuffer(v, &size);
          for (i = 1; i <= (int)R_Journal[j]; i++) v[n++] = R_Journal[j+i];
          }
        }
      }
    }
  }

/* We now have built the replacement string in v */

if (eflag)
  {
  line_deletech(line, match_start, match_end - match_start, TRUE);
  line_insertch(line, match_start, v, n, 0);
  cursor_col = match_start + n;
  }
else
  {
  line_insertch(line, (aflag? match_end:match_start), v, n, 0);
  cursor_col = match_end + n;
  }

store_free(v);
return line;
}

/* End of ematchR.c */
