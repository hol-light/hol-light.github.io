/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 2003 */

/* Written by Philip Hazel, starting November 1991 */

/* This file contains one function, which sets up the current
version string. The main version identity is kept here, but each
system-specific and screen-specific module has its own version
component. Note the fudgery needed to prevent the compiler
optimizing the copyright string out of the binary. */

#ifndef RETYPE
#define RETYPE
#endif

#define str(s) # s
#define xstr(s) str(s)
#define this_version  "1.31" xstr(RETYPE)

#include "ehdr.h"


void version_init(void)
{
int i = 0;
char *copyright = "Copyright (c) University of Cambridge 2003";
char today[20];

version_string = malloc(strlen(copyright));
strcpy(version_string, copyright);
strcpy(version_string, this_version);
sys_setversion(version_string);

strcpy(today, __DATE__);
if (today[4] == ' ') i = 1;
today[3] = today[6] = '-';

version_date = malloc(20);
strcpy(version_date, "(");
strncat(version_date, today+4+i, 3-i);
strncat(version_date, today, 4);
strncat(version_date, today+7, 4);
strcat(version_date, ")");
}

/* End of c.eversion */

