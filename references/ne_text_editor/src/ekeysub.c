/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1993, 1994 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: September 1994 */


/* This file contains code for subroutines connected with
keystroke handling */


#include "ehdr.h"
#include "cmdhdr.h"
#include "keyhdr.h"


static char *keystring;



/*************************************************
*       Subroutines for key_set                  *
*************************************************/

static void keymoan(char *p, char *format, ...)
{
char buff[256];
va_list ap;
va_start(ap, format);
vsprintf(buff, format, ap);
error_moan(14, p - keystring, buff);
va_end(ap);
}


static char *getkey(char *p, int *ak)
{
char *e1 = "   Key name or number expected";
char *e2 = "   \"%s%s\" cannot be configured in this version of E\n   (%s)";
*ak = -1;    /* default bad */

mac_skipspaces(p);

if (*p == 0)
  {
  keymoan(p, e1);
  return p;
  }

/* Deal with a number, indicating a function key */

if (isdigit((unsigned char)(*p)))
  {
  int chcode = 0;
  while (*p != 0 && isdigit((unsigned char)(*p))) 
    chcode = chcode*10 + *p++ - '0';

  if (1 <= chcode && chcode <= max_fkey)
    {
    if ((key_functionmap & (1L << chcode)) == 0)
      keymoan(p, "   Function key %d not available in this version of E", chcode);
    else *ak = chcode + s_f_umax;
    }
  else keymoan(p,"   Incorrect function key number (not in range 1-%d)", max_fkey);
  }

/* Deal with a single letter key name, indicating ctrl/<something> */

else if (!isalpha((unsigned char)(p[1])) && p[1] != '/')
  {
  int chcode = key_codes[(unsigned char)(*p++)];
  if (chcode == 0) keymoan(p, e1);
  else if ((key_controlmap & (1L << chcode)) == 0)
    {
    char name[8];
    strcpy(name, "ctrl/");
    name[5] = p[-1];
    name[6] = 0;
    keymoan(p, e2, "", name, sys_keyreason(chcode));
    }
  else *ak = chcode;
  }

/* Deal with multi-letter key name, possibly preceded by "s/" or "c/" */

else
  {
  char name[20];
  int n = 0;
  int chcode = -1;
  int shiftbits = 0;

  while(p[1] == '/')
    {
    if (*p == 's') shiftbits += s_f_shiftbit; else
      {
      if (*p == 'c') shiftbits += s_f_ctrlbit;
        else keymoan(p, "   s/ or c/ expected");
      }
    p += 2;
    if (*p == 0) break;
    }

  while (isalpha((unsigned char)(*p))) name[n++] = *p++;
  name[n] = 0;

  for (n = 0; key_names[n].name[0] != 0; n++)
    if (strcmp(name, key_names[n].name) == 0)
      { chcode = key_names[n].code; break; }

  if (chcode > 0)
    {
    int mask = (int)(1L << ((chcode-s_f_ubase)/4));

    if ((key_specialmap[shiftbits] & mask) == 0)
      {
      char *sname = "";
      switch (shiftbits)
        {
        case s_f_shiftbit: sname = "s/"; break;
        case s_f_ctrlbit:  sname = "c/"; break;
        case s_f_shiftbit+s_f_ctrlbit: sname = "s/c/"; break;
        }
      keymoan(p, e2, sname, name, sys_keyreason(chcode+shiftbits));
      }
    else *ak = chcode + shiftbits;
    }
  else keymoan(p, "   %s is not a valid key name", name);
  }

return p;
}



static char *getaction(char *p, int *aa)
{
*aa = -1;    /* default bad */

mac_skipspaces(p);

if (*p == 0 || *p == ',') 
  {
  *aa = 0;    /* The unset action */
  }  
   
else if (isalpha((unsigned char)(*p)))
  {
  int n = 0;
  char name[8];
  while (isalpha((unsigned char)(*p))) name[n++] = *p++;
  name[n] = 0;

  for (n = 0; n < key_actnamecount; n++)
    if (strcmp(key_actnames[n].name, name) == 0)
      { *aa = key_actnames[n].code; break; }

  if (*aa < 0) keymoan(p, "   Unknown key action");
  }

else if (isdigit((unsigned char)(*p)))
  {
  int code = 0;
  while (isdigit((unsigned char)(*p))) code = code*10 + *p++ - '0';
  if (1 <= code && code <= max_keystring) *aa = code; else
    keymoan(p, "   Incorrect function keystring number (not in range 1-%d)", max_keystring);
  }

else keymoan(p, "   Key action (letters or a number) expected");

return p;
}


/*************************************************
*    Process setting string for keystrokes       *
*************************************************/

/* This procedure is called twice for every KEY command -
during decoding to check the syntax, and during execution
to do the job. This is a bit wasteful, but it is a rare
enough operation, and it saves having variable-length
argument lists to commands. */

int key_set(char *string, BOOL goflag)
{
char *p = string;

if (!main_screenmode) return TRUE;

keystring = string;

while (*p)
  {
  int a, k;
  p = getkey(p, &k);
  if (k < 0) return FALSE;
  mac_skipspaces(p);
  if (*p != '=' && *p != ':')
    {
    keymoan(p, "   Equals sign or colon expected");
    return FALSE;
    }
  p = getaction(++p, &a);
  if (a < 0) return FALSE;
  mac_skipspaces(p);
  if (*p != 0 && *p != ',')
    {
    keymoan(p, "   Comma expected");
    return FALSE;
    }
  if (*p) p++;

  if (goflag)
    {
    key_table[k] = a;
    }
  }

return TRUE;
}


/*************************************************
*           Generate name of special key         *
*************************************************/

void key_makespecialname(int key, char *buff)
{
char *s1 = "  ";
char *s2 = s1;

buff[0] = 0;
if (sys_makespecialname(key, buff)) return;

if ((key & s_f_shiftbit) != 0) { strcat(buff, "s/"); s1 = ""; }
if ((key & s_f_ctrlbit)  != 0) { strcat(buff, "c/"); s2 = ""; }

strcat(buff, key_specialnames[(key - s_f_ubase) >> 2]);
strcat(buff, s1);
strcat(buff, s2);
}


/* End of ekeysub.c */
