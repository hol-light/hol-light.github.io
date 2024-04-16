/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1994 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: September 1994 */


/* This file contains code for reading search expressions
and qualified strings. */


#include "ehdr.h"


char *cmd_qualletters = "pbehlnrsuvwx";

static int qualbits[] = {
  qsef_B + qsef_E, qsef_B, qsef_E, qsef_H, qsef_L, qsef_N,  
  qsef_R,          qsef_S, qsef_U, qsef_V, qsef_W, qsef_X };

static int qualXbits[] = {
  /* p */ qsef_B + qsef_E + qsef_L,
  /* b */ qsef_B + qsef_E + qsef_L + qsef_H,
  /* e */ qsef_B + qsef_E + qsef_L + qsef_H,
  /* h */ qsef_B + qsef_E + qsef_L + qsef_H + qsef_S,
  /* l */ qsef_B + qsef_E + qsef_L + qsef_H,
  /* n */ 0,
  /* r */ 0,
  /* s */ 0,
  /* u */ qsef_V,
  /* v */ qsef_U,
  /* w */ 0,
  /* x */ 0 };



/*************************************************
*    Pack a (non-R) qualified hex string         *
*************************************************/

/* We retain the original string (for messages),
and construct the packed form in another piece of
store. */

static BOOL hexqs(qsstr *qs)
{
int i;
int len = qs->length;
char *s = qs->text + 1;
char *hex;

if ((len%2) != 0)
  {
  error_moan(18, "character count is odd");
  return FALSE;
  }

qs->hexed = hex = store_Xget(len/2);

for (i = 0; i < len; i += 2)
  {
  int j;
  int x = 0;
  for (j = 0; j <= 1; j++)
    {
    int ch = toupper(*s++);
    x = x << 4;
    if ((ch_tab[ch] & ch_hexch) != 0)
      {
      if (isalpha(ch)) x += ch - 'A' + 10; else x += ch - '0';
      }
    else
      {
      error_moan(19, j, "not a hex digit");
      return FALSE;
      }
    }
  *hex++ = x;
  }

return TRUE;
}




/*************************************************
*      Read qstring with supplied qualifiers     *
*************************************************/

/* This procedure can never fail for ordinary strings,
as we are already pointing at a valid delimiter character
on entry. However, it can fail to compile a regular
expression or to construct the packed form of a hex
string. The last argument is a flag indicating whether
it is a replacement string that is being read. In this
case, the R qualifier does not cause compilation. */

static BOOL readsq(qsstr **a_qs, int count, int flags, int wleft, int wright, int rflag)
{
int i, n;
int dch = *cmd_ptr++;
char *p = cmd_ptr;
qsstr *qs;

while (*p != 0 && *p != dch) p++;

if (*p == 0 && dch != ';') cmd_ist = dch;
n = p - cmd_ptr;

qs = store_Xget(sizeof(qsstr));
qs->type = cb_qstype;
qs->count = count;
qs->windowleft = wleft;
qs->windowright = wright;
qs->length = qs->rlength = n;
qs->fsm = NULL;
qs->hexed = NULL;
p = qs->text = qs->rtext = store_Xget(n+2);

*p++ = dch;
for (i = 0; i < n; i++) *p++ = *cmd_ptr++;
*p++ = 0;

if (*cmd_ptr) cmd_ptr++;
mac_skipspaces(cmd_ptr);

*a_qs = qs;

if (n == 0) flags &= ~qsef_R;        /* no null REs */
if ((flags & qsef_X) != 0)           /* hex => exact */
  flags = (flags & ~qsef_U) | qsef_V;

qs->flags = flags;

/* If this is not a replacement string, we must set up the
bit map indicating which characters are present in the string.
This speeds up searching no end. Before doing so, we must
enhex hex strings, so as to make the map from the hexed data. 
The bitmap is not used for regular expressions, but in that
case we must compile the finite state machine. */

if ((flags & qsef_R) != 0) return rflag? TRUE : cmd_makeFSM(qs);

if ((flags & qsef_X) != 0) 
  { 
  if (!hexqs(qs)) return FALSE;
  p = qs->hexed;
  n /= 2;
  } 
else p = qs->text + 1;

/* We have to set the bits in a caseless manner, as we don't know
at this time whether the match will be cased or not. */

if (!rflag)
  {
  memset((void *)qs->map, 0, sizeof(qs->map));
  while (n-- > 0)
    {
    unsigned char c = (unsigned char)(*p++); 
    c = toupper(c); 
    qs->map[c/intbits] |= 1 << (c%intbits);
    c = tolower(c); 
    qs->map[c/intbits] |= 1 << (c%intbits);
    }
  } 

return TRUE;
}



/*************************************************
*             Read qualifiers                    *
*************************************************/

/* The qualifiers must be terminated by a legal string
delimiter character, or '(' if the second argument is FALSE.
Otherwise, they must be terminated by '?'. The qualifier
string may be null in the normal case, but not when part of
a prompted argument. No error messages are given if the
second argument is TRUE, as the string will be parsed again
as a non-prompting argument. */

static BOOL readqual(int *a_count, int *a_flags, int *a_wleft, int *a_wright, BOOL prompttest)
{
BOOL countread = FALSE;
int ch = tolower(*cmd_ptr);
int count = 1;
int flags = 0;
int wleft = qse_defaultwindowleft;
int wright = qse_defaultwindowright;

if (prompttest && ch == '?') return FALSE;

/* Grand Loop */

for (;;)
  {

  /* We are at a valid qualifier letter */

  if ((ch_tab[ch] & ch_qualletter) != 0)
    {
    int p = strchr(cmd_qualletters, ch) - cmd_qualletters;
    int q = qualbits[p];
    int r = qualXbits[p];

    /* Most checking for illegal combinations is done simply
    using the tables. However, we must check explicitly for H
    following P because P = B+E and the tables forbid PB and PE. */

    if (ch == 'h' && (flags & qsef_EB) == qsef_EB) r &= ~qsef_EB;

    /* Check permitted combinations */

    if ((flags & (q | r)) != 0)
      {
      if (!prompttest) error_moan(20);
      return FALSE;
      }

    /* Accept this qualifier */

    flags |= q;
    cmd_ptr++;
    }

  /* Not a qualifier letter - try for digit */

  else if ((ch_tab[ch] & ch_digit) != 0)
    {
    count = cmd_readnumber();
    if (countread)
      {
      if (!prompttest) error_moan(20);
      return FALSE;
      }
    countread = TRUE;
    }

  /* Not a letter or digit - try for column qualifier */

  else if (ch == '[' || ch == '_')
    {
    if (wleft != qse_defaultwindowleft || wright != qse_defaultwindowright)
      {
      if (!prompttest) error_moan(20);
      return FALSE;
      }

    cmd_ptr++;
    wleft = cmd_readnumber();
    wright = wleft;
    if (wleft > 0) wleft--; else wleft = 0;

    ch = *cmd_ptr;
    if (ch == ',')
      {
      cmd_ptr++;
      wright = cmd_readnumber();
      ch = *cmd_ptr;
      }

    if (ch != ']' && ch != '_')
      {
      if (!prompttest) error_moan(13, "\"]\"");
      return FALSE;
      }
    cmd_ptr++;

    if (wright < 0) wright = qse_defaultwindowright;
    }

  /* Not a recognized qualifier in any form; try for valid end
  of qualifiers. */

  else if ((prompttest && ch == '?') ||
        (!prompttest && (ch == '(' || (ch_tab[ch] & ch_delim) != 0)))
    {
    if (countread)
      {
      if ((flags & qsef_EB) != 0)
        {
        if (!prompttest) error_moan(20);
        return FALSE;
        }
      }

    /* Hand back qualifiers via address arguments */

    *a_count = count;
    *a_flags = flags;
    *a_wleft = wleft;
    *a_wright = wright;

    return TRUE;
    }

  /* Neither a valid qualifier, nor a valid terminator */

  else
    {
    if (!prompttest) error_moan(13, "String");
    return FALSE;
    }

  /* Valid qualifier read, but expect something to follow */

  if (cmd_atend())
    {
    if (!prompttest) error_moan(13, "String");
    return FALSE;
    }

  /* Get the next character and loop */

  ch = tolower(*cmd_ptr);
  }
}


/*************************************************
*           Read a qualified string              *
*************************************************/

/* This procedure is not used when reading search
expressions, as the qualifiers are then separately
read, in case they precede a bracket. */

BOOL cmd_readqualstr(qsstr **a_qs, int rflag)
{
int count, flags, wleft, wright;
mac_skipspaces(cmd_ptr);
if (!readqual(&count, &flags, &wleft, &wright, FALSE)) return FALSE;

/* When replacement string, only some qualifiers are legal */

if (rflag)
  {
  int t = flags;

  switch (rflag)
    {
    case rqs_XRonly: t &= ~qsef_R;
    case rqs_Xonly:  t &= ~qsef_X;
    }

  if (count != 1 || t != 0 ||
      wleft != qse_defaultwindowleft || wright != qse_defaultwindowright)
    {
    error_moan(21, (rflag == rqs_Xonly)? "x" : "x or r");
    return FALSE;
    }
  }

return readsq(a_qs, count, flags, wleft, wright, rflag);
}


/*************************************************
*            Read Search Expression              *
*************************************************/


/* Internal subroutine to read a group of qualified strings
connected by '&' and to build them into a tree. */

static BOOL readANDseq(sestr **a_se)
{
sestr *yield;
if (!cmd_readse(&yield)) return FALSE;

while (*cmd_ptr == '&')
  {
  sestr *new = store_Xget(sizeof(sestr));
  new->type = cb_setype;
  new->count = 1;
  new->flags = qsef_AND;
  new->windowleft = qse_defaultwindowleft;
  new->windowright = qse_defaultwindowright;
  new->left.se = yield;
  new->right.se = NULL;
  yield = new;
  cmd_ptr++;
  if (!cmd_readse(&yield->right.se))
    {
    cmd_freeblock((cmdblock *)yield);
    return FALSE;
    }
  }

*a_se = yield;
return TRUE;
}


/* Start of Readse proper */

/* The yield is TRUE if data was successfully read. The returned
control block is either a search expression or a qualified string. */

BOOL cmd_readse(sestr **a_se)
{
int count, flags, wleft, wright;
mac_skipspaces(cmd_ptr);
if (!readqual(&count, &flags, &wleft, &wright, FALSE)) return FALSE;

/* Qualifier read; now read a search expression after "(", first checking
for forbidden qualifiers. */

if (*cmd_ptr == '(')
  {
  int op;
  sestr *left;

  if ((flags & qsef_NotSe) != 0 || count != 1 ||
      wleft  != qse_defaultwindowleft || wright != qse_defaultwindowright)
    {
    error_moan(22);
    return FALSE;
    }

  /* Read sequence connected by '&' (most binding) */

  cmd_ptr++;
  if (!readANDseq(&left)) return FALSE;

  /* Now deal with '|' */

  while ((op = *cmd_ptr++) == '|')
    {
    sestr *right;
    if (readANDseq(&right))
      {
      sestr *x = store_Xget(sizeof(sestr));
      x->type = cb_setype;
      x->count = 1;
      x->flags = 0;
      x->windowleft = wleft;
      x->windowright = wright;
      x->left.se = left;
      x->right.se = right;
      left = x;
      }
    else
      {
      cmd_freeblock((cmdblock *)left);
      return FALSE;
      }
    }

  /* Check for valid end of search expression */

  if (op == ')')
    {
    mac_skipspaces(cmd_ptr);

    /* If we have read a single search expression in brackets,
    convert it to an OR node with a zero right hand address,
    to ensure that matching yields a complete line. */

    if (left->type == cb_qstype)
      {
      sestr *node = store_Xget(sizeof(sestr));
      node->type = cb_setype;
      node->count = 1;
      node->flags = 0;
      node->windowleft = qse_defaultwindowleft;
      node->windowright = qse_defaultwindowright;
      node->left.se = left;
      node->right.se = NULL;
      left = node;
      }

    /* Set qualifiers for the whole search expression */

    left->flags |= flags;
    left->windowleft = wleft;
    left->windowright = wright;

    /* Pass back address and return success */

    *a_se = left;
    return TRUE;
    }

  /* Invalid search expression */

  else
    {
    cmd_freeblock((cmdblock *)left);
    error_moan(13, "\"&\" or \"|\"");
    return FALSE;
    }
  }

/* Not a search expression */

else return readsq((qsstr **)a_se, count, flags, wleft, wright, FALSE);
}

/* End of erdseqs.c */
