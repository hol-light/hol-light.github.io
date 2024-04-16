/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1993, 1994 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: September 1994 */


/* This file contains code for compiling a regular expression */

/* #define UNIXREDEBUG */

#include "ehdr.h"
#include "rehdr.h"


/* Variables used in compiling and matching. Actually, do these have to
be external? Each module could have its own. */

unsigned char *R_String;
unsigned char *R_States;
unsigned char *R_Sflags;
unsigned char *R_Journal;
unsigned char *R_List;

int R_ListPtr;
int R_Bracount;
int R_Jptr;

/* For the Unix conversion routines */

static char *re_inptr, *re_inend, *re_inbegin;


/* Mutual recursion occurs */

static int  makeFSM_exp(BOOL, int, qsstr *);
static BOOL makeFSM_next(qsstr *);
static int  makeFSM_prim(qsstr *);
static int  re_convert1(char *);



/*************************************************
*      Get next char and test for end            *
*************************************************/

static int makeFSM_nextchar()
{
if (++R_ListPtr > (int)R_String[0])
  { error_moan(38, R_ListPtr, "unexpected end of expression"); return -1; }
return R_String[R_ListPtr];
}


/************************************************
*  Compile Regular Expression: Set exit list    *
************************************************/

static void makeFSM_set(int ptr, int value)
{
while (ptr != 0)
  {
  int next = R_States[ptr];
  R_States[ptr] = value;
  ptr = next;
  }
}




/*************************************************
*     Syntax check a negative alternative        *
*************************************************/

/* The 'not' sign is allowed in front of an alternative bracket,
provided that each alternative is a single character test. The
whole thing counts as a one-character test and is processed
specially in the matching routine. This routine checks that the
appropriate conditions are fulfilled. */

static BOOL makeFSM_negalt(qsstr *qs)
{
int rangestart = -1;

  /* Loop to check alternatives */

for (;;)
  {
  int ch, nextch;
  BOOL onehex = FALSE;
  if ((ch = makeFSM_nextchar()) < 0) return FALSE;
  if (ch == '$')
    {
    if ((ch = makeFSM_nextchar()) < 0) return FALSE;
    ch = tolower(ch);
    if (ch == 'h')
      {
      if ((ch = makeFSM_nextchar()) < 0) return FALSE;
      onehex = TRUE;
      }
    else if (ch != 'a' && ch != 'd' && ch != 'l' && ch != 'p' && ch != 's' && ch != 'w')
      {
      error_moan(38, R_ListPtr, "$ must be followed by a, d, l, p, s, w, or h"
        " (in upper or lower case)");
      return FALSE;
      }
    }
  else if (ch == '?' || ch == '#' || ch == '^' || ch == '(' ||
        ch == ')' || ch == '|' || ch == '~' || ch == '-')
    {
    char buff[64];
    sprintf(buff, "\"%c\" not allowed here", ch);
    error_moan(38, R_ListPtr, buff);
    return FALSE;
    }
  else if (ch == '\"')
    {
    if ((ch = makeFSM_nextchar()) < 0) return FALSE;
    }

  /* Complete hex byte if necessary */

  if ((qs->flags & qsef_X) != 0 || onehex)
    {
    int b;
    if ((b = makeFSM_nextchar()) < 0) return FALSE;
    if (!isxdigit(ch) || !isxdigit(b))
      {
      error_moan(38, R_ListPtr, "not a hex digit");
      return FALSE;
      }
    ch = mac_hex(ch, b);
    }

  /* Get next char -- there has to be one */

  if ((nextch = makeFSM_nextchar()) < 0) return FALSE;

  /* Deal with ranges */

  if (rangestart >= 0)
    {
    if (ch < rangestart)
      {
      error_moan(38, R_ListPtr, "range out of order");
      return FALSE;
      }
    rangestart = -1;
    }
  else if (nextch == '-') { rangestart = ch; continue; }

  /* Check for another alternative or end of bracket */

  if (nextch == '|') continue;
  if (nextch == ')') break;

  error_moan(38, R_ListPtr, "only single-character alternatives"
    " are allowed after NOT");
  return FALSE;
  }

R_ListPtr++;   /* move past ')' */
return TRUE;
}



/*************************************************
*  Compile Regular Expr: Advance past next atom  *
*************************************************/

static int makeFSM_next(qsstr *qs)
{
BOOL onehex = FALSE;    /* flag for individual hex char (range) */
int ch = R_String[R_ListPtr];

if (ch == '#' || ch == '^' || ch == '(')
  {
  R_ListPtr++;
  return TRUE;
  }

R_Sflags[R_ListPtr] |= R_MatchState;    /* this must be a match state */

if (ch == '?') { R_ListPtr++; return TRUE; }

/* If '~' then check legal and bump to next, but deal with
negative alternatives separately. */

if (ch == '~')
  {
  if ((ch = makeFSM_nextchar()) < 0) return FALSE;
  if (ch == ')' || ch == '~' || ch == '#' || ch == '^' || ch == '?')
    {
    char buff[64];
    sprintf(buff, "\"%c\" not allowed after \"~\"", ch);
    error_moan(38, R_ListPtr, buff);
    return FALSE;
    }
  if (ch == '(') return makeFSM_negalt(qs);   /* negative alternative */
  }

/* If '$' then check for legal code char; if 'h' then set flag to
make use of hex checking code below. */

if (ch == '$')
  {
  if ((ch = makeFSM_nextchar()) < 0) return FALSE;
  ch = tolower(ch);

  if (ch == 'a' || ch == 'd' || ch == 'l' || ch == 'p' || ch == 's' || ch == 'w')
    { R_ListPtr++; return TRUE; }

  else if (ch == 'h')
    {
    if ((ch = makeFSM_nextchar()) < 0) return FALSE;
    onehex = TRUE;
    }
  else
    {
    error_moan(38, R_ListPtr, "$ must be followed by a, d, l, p, s, w or h "
      " (in upper or lower case)");
    return FALSE;
    }
  }

/* If quote, then bump to next */

else if (ch == '\"')
  { if ((ch = makeFSM_nextchar()) < 0) return FALSE; }

/* Deal with hex char */

if ((qs->flags & qsef_X) != 0 || onehex)
  {
  int b = ch;
  if ((ch = makeFSM_nextchar()) < 0) return FALSE;
  if (!isxdigit(ch) || !isxdigit(b))
    {
    error_moan(38, R_ListPtr, "not a hex digit");
    return FALSE;
    }
  ch = mac_hex(ch, b);
  }

/* Bump pointer to point after the char */

R_ListPtr++;

/* If the next char is '-', check legality of range of
characters, and move past it, else return. */

if (R_ListPtr > (int)R_String[0] || R_String[R_ListPtr] != '-') return TRUE;

if ((qs->flags & qsef_X) != 0 || onehex)
  {
  /* In the case of a hex string, we check for valid digits
  and that the first char is less than the second. */

  int a, b;
  if ((a = makeFSM_nextchar()) < 0) return FALSE;
  if ((b = makeFSM_nextchar()) < 0) return FALSE;
  if (!isxdigit(a) || !isxdigit(b))
    {
    error_moan(38, R_ListPtr, "not a hex digit");
    return FALSE;
    }
  a = mac_hex(a, b);
  if (ch > a)
    {
    error_moan(38, R_ListPtr, "range out of order");
    return FALSE;
    }
  R_ListPtr++;
  }

else
  {
  /* For a non-hex string, check that the two chars are both
  letters or both digits, and that the first precedes the
  second in collating sequence. */

  int last, lastbits;
  int first = ch;
  int firstbits = isalpha(ch) + (isdigit(ch) << 1);
  if (firstbits == 0)
    {
    error_moan(38, R_ListPtr, "\"-\" only available with letters or digits");
    return FALSE;
    }
  if ((last = makeFSM_nextchar()) < 0) return FALSE;
  lastbits = isalpha(last) + (isdigit(last) << 1);
  if (lastbits == 0)
    {
    error_moan(38, R_ListPtr, "\"-\" only available with letters or digits");
    return FALSE;
    }
  if (firstbits != lastbits)
    {
    error_moan(38, R_ListPtr, "mixed range not allowed");
    return FALSE;
    }
  if (first > last)
    {
    error_moan(38, R_ListPtr, "range out or order");
    return FALSE;
    }
  R_ListPtr++;              /* past end char */
  }

return TRUE;
}


/*************************************************
*     Compile Regular Expression: <prim>         *
*************************************************/

static int makeFSM_prim(qsstr *qs)
{
int primstart = R_ListPtr;
int ch;
if (R_ListPtr > (int)R_String[0])
  {
  error_moan(38, R_ListPtr, "unexpected end of expression");
  return -1;
  }
ch = R_String[R_ListPtr];

if (ch == ')' || ch == '-' || ch == '|')
  {
  char buff[64];
  sprintf(buff, "\"%c\" not allowed here", ch);
  error_moan(38, R_ListPtr, buff);
  return -1;
  }

if (!makeFSM_next(qs)) return -1;      /* advance to next atom */

/* If preceding char was '~' or '"' then set exit point to
the following position. */

if (ch == '~' || ch == '\"') primstart++;

/* If preceding char was '#' then read next prim and set
its exits to loop back here. */

else if (ch == '#' || ch == '^')
  {
  int prim = makeFSM_prim(qs);
  if (prim < 0) return -1;
  makeFSM_set(prim, primstart);
  }

/* else deal with groups */

else if (ch == '(')
  {
  BOOL nullalternative = FALSE;
  while (R_ListPtr <= (int)R_String[0] && R_String[R_ListPtr] == '|')
    {
    R_ListPtr++;
    nullalternative = TRUE;
    }
  R_Bracount++;
  primstart = makeFSM_exp(nullalternative, primstart, qs);
  if (primstart < 0) return -1;
  R_Bracount--;
  if (R_ListPtr > (int)R_String[0] || R_String[R_ListPtr] != ')')
    {
    error_moan(38, R_ListPtr, "\")\" expected");
    return -1;
    }
  R_ListPtr++;              /* move past ) */

  /* If top level alternation group, mark end. If the next character
  is ^ we must put the flag one item later, as that is where control
  goes when it exits from the alternation group. */

  if (R_Bracount == 0)
    {
    int offset = (R_String[R_ListPtr] == '^')? 1 : 0;
    R_Sflags[R_ListPtr + offset] |= R_Endalt;
    }
  }

return primstart;
}



/*************************************************
*      Compile Regular Expression: <exp>         *
*************************************************/

static int makeFSM_exp(BOOL nullalternative, int altptr, qsstr *qs)
{
int exitchain = -1;    /* no internal exit points yet */
int len = R_String[0];

  /* Loop reading primitives until end of string or alternation
  or closing bracket. After an alternation, loop resumes. */

for (;;)
  {
  int PrimExits = makeFSM_prim(qs);
  int ch = (R_ListPtr > len)? 0 : R_String[R_ListPtr];
  if (PrimExits < 0) return -1;  /* Error state */

  /* If we are not at the end of an alternation, or the end
  of the whole thing (which is of course the end of an
  alternation), just set the exits in the primitive to the
  current state -- except when the current character is '^',
  when we point one further on. */

  if (R_ListPtr <= len && ch != '|' && (R_Bracount <= 0 || ch != ')'))
      {
      makeFSM_set(PrimExits, (ch == '^')? R_ListPtr+1 : R_ListPtr);
      continue;    /* to read next <prim> */
      }

  /* At end of string or end of alternation (either '|' or ')')
  add exits for the primitive to existing exits list. */

  if (exitchain >= 0) R_States[PrimExits] = exitchain;
  exitchain = PrimExits;

  /* Unless at start of new alternation, return. A null
  alternative at the start or in the middle of an <exp>
  is dealt with here -- that is, it is treated as a null
  alternative at the end of the group. */

  if (R_ListPtr <= len && R_String[R_ListPtr] == '|')
    {
    R_States[altptr] = R_ListPtr;    /* add to alternation chain */
    altptr = R_ListPtr;              /* and continue */
    }
  else if (nullalternative)
    {
    R_States[altptr] = R_ListPtr;    /* add to alternation chain */
    altptr = R_ListPtr;              /* and exit */
    break;
    }
  else break;                        /* for ')' just exit */

  /* Now check for a null alternative here */

  for (;;)
    {
    R_ListPtr++;         /* advance past '||' */
    if (R_ListPtr > len || R_String[R_ListPtr] == ')') return exitchain;
    if (R_String[R_ListPtr] == '|') nullalternative = TRUE; else break;
    }
  }

return exitchain;
}



/*************************************************
*  Unix Conversion: Get next one-character/item  *
*************************************************/

static int re_nextone(char *out)
{
char *outptr = out;
int ch = *re_inptr++;

switch (ch)
  {
  case '+': case '?': case '*':
  break;

  case '|': case ')':
  re_inptr--;
  break;

  case '\\':
  ch = *re_inptr++;
  if (isalpha(ch)) switch(ch)
    {
    case 'd': case 's': case 'w':
    sprintf(outptr, "$%c", ch);
    outptr += 2;
    break;

    case 'D': case 'S': case 'W':
    sprintf(outptr, "~$%c", ch);
    outptr += 3;
    break;

    default:
    error_moan(38, re_inptr - re_inbegin, "Unknown letter after \"\\\"");
    return -1;
    }
  else
    {
    *outptr++ = '\"';
    *outptr++ = ch;
    }
  break;

  case '.':
  *outptr++ = '?';
  break;

  case '[':
  if (re_inptr < re_inend && *re_inptr == '^')
    { *outptr++ = '~'; re_inptr++; }
  *outptr++ = '(';

  while (re_inptr < re_inend)
    {
    if (!isalnum((unsigned char)(*re_inptr))) *outptr++ = '\"';
    *outptr++ = *re_inptr++;
    if (re_inptr < re_inend - 1 && *re_inptr == '-' && re_inptr[1] != ']')
      {
      *outptr++ = *re_inptr++;
      if (!isalnum((unsigned char)(*re_inptr))) *outptr++ = '\"';
      *outptr++ = *re_inptr++;
      }
    if (re_inptr >= re_inend || *re_inptr == ']')
      {
      re_inptr++;
      *outptr++ = ')';
      break;
      }
    *outptr++ = '|';
    }
  break;

  case '(':
  *outptr++ = '(';
    {
    int temp = re_convert1(outptr);
    if (temp < 0) return temp;
    outptr += temp;
    }
  *outptr++ = ')';
  if (*re_inptr == ')') re_inptr++;
  break;

  case '$': case '\"':  case '#': case '^': case '-': case '~':
  *outptr++ = '\"';
  *outptr++ = ch;
  break;

  default:
  *outptr++ = ch;
  break;
  }

return outptr - out;
}



/*************************************************
* Unix Conversion: Convert a bracketed RE segment*
*************************************************/

/* Read to the end of the RE or the next closing bracket. */

static int re_convert1(char *out)
{
char *outptr = out;
char temp[256];

/* The main loop reads and converts the next one-character RE, and then
adds it to the output in a form determined by the following character. */

while (re_inptr < re_inend)
  {
  char *add = "";
  int newlen = re_nextone(temp);
  if (newlen < 0) return newlen;
  if (re_inptr < re_inend) switch (*re_inptr)
    {
    case '+': *outptr++ = '^'; re_inptr++; break;
    case '*': *outptr++ = '#'; re_inptr++; break;
    case '?': *outptr++ = '('; re_inptr++; add = "|)"; break;
    }

  memcpy(outptr, temp, newlen);
  outptr += newlen;
  sprintf(outptr, "%s", add);
  outptr += strlen(add);

  if (re_inptr < re_inend)
    {
    if (*re_inptr == ')') break;
    if (*re_inptr == '|') *outptr ++ = *re_inptr++;
    }
  }

return outptr - out;
}



/*************************************************
*          Convert Unix Regular Expression       *
*************************************************/

/* This function converts Unix-style regular expressions into the parochial
form used by NE - which is an historical inheritance. */

static BOOL cmd_convert_unix_RE(qsstr *qs)
{
char out[256];

re_inbegin = re_inptr = qs->text + 1;       /* past the delimiter */
re_inend = qs->text + qs->length + 1;

/* An initial ^ causes the B flag to be set. */

if (re_inptr < re_inend && *re_inptr == '^')
  {
  qs->flags |= qsef_B;
  re_inptr++;
  }

/* Do the conversion */

if ((qs->rlength = re_convert1(out)) < 0) return FALSE;

/* A final literal $ causes the E flag to be set, unless it was
a literal $ in the original. */

if (qs->rlength >= 2 && strncmp(out + qs->rlength - 2, "\"$", 2) == 0 &&
  strncmp(qs->text + qs->length + 1 - 2, "\\$", 2) != 0)
  {
  qs->flags |= qsef_E;
  qs->rlength -= 2;
  }

/* Save the converted expression. */

qs->rtext = store_Xget(qs->length + 1);
qs->rtext[0] = qs->text[0];    /* copy the delimiter for tidiness */
memcpy(qs->rtext + 1, out, qs->rlength);
return TRUE;
}



/*************************************************
*           Compile a Regular Expression         *
*************************************************/

/* The code was originally written to handle E's own style of RE,
developed a long time ago. If the Unix Regular Expression flag is
set, we translate Unix-style REs into E-style REs first. */

BOOL cmd_makeFSM(qsstr *qs)
{
int i, len;
char *s;

/* Convert the expression if necessary */

if (main_unixregexp)
  {
  if (!cmd_convert_unix_RE(qs)) return FALSE;

  #ifdef UNIXREDEBUG
  for (i = 0; i <= qs->length; i++) debug_printf("%c", qs->text[i]);
  debug_printf("%c  =>  ", qs->text[0]);
  for (i = 0; i <= qs->rlength; i++) debug_printf("%c", qs->rtext[i]);
  debug_printf("%c\n", qs->rtext[0]);
  #endif
  }

/* We now have the RE in the original E style. */

len = qs->rlength;
s = qs->rtext + 1;       /* Past the delimiter */

/* Get store for FSM data if necessary (4 vectors) */

if (qs->fsm == NULL) qs->fsm = store_Xget(len*4+4);

/* Set up global pointers to avoid too much argument and address
passing, and zero the flag bytes. */

R_String = qs->fsm;
R_States = R_String + len + 1;
R_Sflags = R_States + len + 1;
R_List   = R_Sflags + len + 1;

R_String[0] = len;
for (i = 0; i <= len; i++)
  {
  R_States[i] = 0;
  R_Sflags[i] = 0;
  }

/* Copy the string to R_String, reversing if necessary, and if
reversed, set a flag to indicate this fact. Note that the string
is set up in BCPL format - with a length byte first instead of
a zero byte at the end. This is because this code was originally
written in BCPL, and it is much simpler to keep the logic unchanged
as we convert it to C. */

if ((qs->flags & qsef_L) != 0 ||
    ((match_L || (qs->flags & qsef_E) != 0) && (qs->flags & qsef_B) == 0))

  /* Reversing is required; use a stack for reversing items */

  {
  char stack[100];
  int stackptr = 0;
  int p = len;                 /* initial posn */
  BOOL quoted = FALSE;         /* flag for quoted chars */
  BOOL onehex = FALSE;         /* flag for $h pairs */

  stack[stackptr] = '(';       /* to force out at end */

  /* Inside the following FOR loop, the loop index is explicitly
  altered when range items are processed. */

  for (i = 0; i < len; i++)
    {
    int ch = s[i];

    /* If last char, check for unexpected end */

    if (i == len-1 && !quoted && (ch == '\"' || ch == '#' || ch == '~' ||
      ch == '^' || ch == '$' || ch == '('))
        { error_moan(38, i, "unexpected end"); return FALSE; }

    /* If not the last char, and not quoted, test for items that
    must be reversed and stack them. */

    if (i < len-1 && !quoted)
      {
      if (ch == '\"')
        {
        stack[++stackptr] = ch;
        quoted = TRUE;
        continue;
        }
      else if (ch == '#' || ch == '~' || ch == '^')
        {
        stack[++stackptr] = ch;
        continue;
        }
      else if (ch == '$')
        {
        stack[++stackptr] = ch;
        ch = s[i+1];
        if (tolower(ch) == 'h' && i <= len-4)
          {
          stack[++stackptr] = ch;
          i++;
          onehex = TRUE;
          }
        continue;
        }
      else if (ch == '(')
        {
        stack[++stackptr] = ch;
        R_String[p--] = ')';    /* reverse bracket in string */
        continue;
        }
      }

    /* Not a stackable char, or quoted char */

    if (ch == ')' && !quoted)
      {
      R_String[p--] = '(';      /* reverse bracket in string */
      if (stack[stackptr] != '(')
        {
        error_moan(38, i, "unexpected \")\"");
        return FALSE;
        }
      if (stackptr <= 0)
        {
        error_moan(38, i, "too many closing brackets");
        return FALSE;
        }
      stackptr--;              /* move past ( */
      }

    /* Data char (possibly quoted) */

    else
      {
      quoted = FALSE;
      if ((qs->flags & qsef_X) != 0 || onehex)
        {
        stack[++stackptr] = ch;  /* stack first nibble */
        i++;
        }

      R_String[p--] = ch;

      /* Deal with specification of character range */

      if (i < len-2 && s[i+1] == '-')
        {
        stack[++stackptr] = R_String[p+1];  /* stack first char */
        stack[++stackptr] = '-';            /* and the - */
        i += 2;                             /* look at next char */
        if (i < len-1 && s[i] == '\"')
          {
          stack[++stackptr] = '\"';         /* stack quote */
          i++;                              /* bump to next */
          }
        if ((qs->flags & qsef_X) != 0 || onehex)
          {
          stack[++stackptr] = s[i];         /* stack first nibble */
          i++;
          }
        R_String[p+1] = s[i];               /* 2nd char here */
        }

      onehex = FALSE;
      }

    /* After processing a data char, flush the reversing stack */

    while (stack[stackptr] != '(') R_String[p--] = stack[stackptr--];
    }

  qs->flags |= qsef_REV;  /* indicate reversed */
  }

/* Forward processing -- just a straight copy from C-style to BCPL style.
We must clear the reverse flag, in case previously set. */

else
  {
  memcpy((char *)R_String+1, s, len);
  qs->flags &= ~qsef_REV;
  }


/* Now we compile the string to produce a state list. We can now
process from left to right in all cases. */

  {
  BOOL nullalternative = FALSE;
  R_ListPtr = 1;   /* First state */
  R_Bracount = 0;

  /* Deal with null alternatives at start */

  while (R_String[R_ListPtr] == '|')
    {
    R_ListPtr++;
    nullalternative = TRUE;
    }

  /* Read top level expression and set its exits to "end" */

    {
    int exp = makeFSM_exp(nullalternative, 0, qs);
    if (exp < 0) return FALSE;
    makeFSM_set(exp, 0);
    }
  }

/* Debugging printing statements */

#ifdef REDEBUG
debug_printf("------------------------------------------\n");
for (i = 0; i <= R_String[0]; i++) debug_printf("%3d", i);
debug_printf("\n   ");
for (i = 1; i <= R_String[0]; i++) debug_printf("  %c", R_String[i]);
debug_printf("\n");
for (i = 0; i <= R_String[0]; i++) debug_printf("%3d", R_States[i]);
debug_printf("\n");
for (i = 0; i <= R_String[0]; i++) debug_printf("  %s",
  (R_Sflags[i] & R_MatchState)? "*" : " ");
debug_printf("\n");
for (i = 0; i <= R_String[0]; i++) debug_printf("  %s",
  (R_Sflags[i] & R_Endalt)? "!" : " ");
debug_printf("\n");
#endif

return TRUE;   /* success */
}

/* End of ecompR.c */
