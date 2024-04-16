/*************************************************
*                Wimp Subroutines                *
*************************************************/

/* Subroutines for doing wimp-ish things. */
/* This file last changed: June 1994 */

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "roshdr.h"

#define FALSE  0
#define TRUE   1


/*************************************************
*              Global variables                  *
*************************************************/

_kernel_swi_regs swi_regs;
char *Application_Name = "?";



/*************************************************
*             Display error message              *
*************************************************/

/* Calls the SWI directly so that it can be used from SWI(). */

void display_error(char *format, ...)
{
_kernel_swi_regs tempregs;
_kernel_oserror error;
va_list ap;
va_start(ap, format);
error.errnum = 0;
vsprintf(error.errmess, format, ap);
va_end(ap);
tempregs.r[0] = (int)(&error);
tempregs.r[1] = 1;
tempregs.r[2] = (int)Application_Name;
_kernel_swi(Wimp_ReportError, &tempregs, &tempregs);
}



/*************************************************
*          Call SWI; fail if error               *
*************************************************/

int SWI(int swi, int count, ...)
{
int i;
_kernel_oserror *error;
va_list ap;
va_start(ap, count);
for (i = 0; i < count; i++) swi_regs.r[i] = va_arg(ap, int);
va_end(ap);
error = _kernel_swi(swi, &swi_regs, &swi_regs);

if (error != NULL)
  {
  display_error("Error while obeying SWI &%x: %s", swi, error->errmess);
  exit(99); 
  } 

return swi_regs.r[0];  /* Commonly needed value */
}



/*************************************************
*      Get the (indirected) text of an icon      *
*************************************************/

/* Ensure it is terminated by zero. This is only ever called
for indirected icons, which always contain a terminator. */

char *get_icon_text(int handle, int icon)
{
char *s;
icon_state_block isb;
isb.handle = handle;
isb.icon = icon;
SWI(Wimp_GetIconState, 2, 0, (int)(&isb));
s = isb.ib.id.ind.text;
while (*s >= 32) s++;
*s = 0;
return isb.ib.id.ind.text;
}



/*************************************************
*      Set the (indirected) text of an icon      *
*************************************************/

void set_icon_text(int handle, int icon, char *string)
{
int flags, length;
int setblock[4];

icon_state_block isb;
isb.handle = setblock[0] = handle;
isb.icon = setblock[1] = icon;

SWI(Wimp_GetIconState, 2, 0, (int)(&isb));

flags = isb.ib.flags;

length = isb.ib.id.ind.length;
strncpy(isb.ib.id.ind.text, string, length);
if (strlen(string) + 1 < length) length = strlen(string) + 1;
isb.ib.id.ind.text[length-1] = '\r';

setblock[2] = flags;
setblock[3] = -1;
SWI(Wimp_SetIconState, 2, 0, (int)(setblock));
}



/*************************************************
*              Load a window template            *
*************************************************/

int load_template(char *name, window_data **ptr)
{
int *indirectws = NULL;
int iwssize = 0;
int fixed_name[3];

/* Set up the name as a 12-byte, word-aligned string */

fixed_name[0] = fixed_name[1] = fixed_name[2] = 0;
strcpy((char *)fixed_name, name);

/* Find the size of buffer needed */

SWI(Wimp_LoadTemplate, 7, 0, -1, 0, 0, 0, (int)fixed_name, 0);

/* Get main and auxiliary buffers */

*ptr = malloc(swi_regs.r[1]);
if (swi_regs.r[2] > 0)
  {
  iwssize = swi_regs.r[2];
  indirectws = malloc(iwssize);
  }

/* Load the template */

SWI(Wimp_LoadTemplate, 7, 0, (int)(*ptr), (int)indirectws,
  (int)(indirectws + iwssize), (int)(-1), (int)fixed_name, 0);

if (swi_regs.r[6] == 0)
  {
  display_error("Failed to load template for \"%s\"", fixed_name);
  return FALSE;
  }

return TRUE;
}

/* End of wimpsubs.c */
