/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1992, 1993 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: September 1994 */

/* This file is specific to the support of SunOS 4. It contains
functions that are specific to the particular operating system,
or compiler, or whatever. */

#include "ehdr.h"


/* Under SunOS 4 we were originally using the gnu compiler and the
normal libraries. They don't seem to have strerror(). This is still
true using acc. */

char *strerror(int n)
{
if (n >= sys_nerr) return "unknown error number";
return sys_errlist[n];
}

/* Likewise, there is no memmove() (move with overlapping
behaviour defined). NE uses this only rarely, so don't try
to be clever. The acc compiler does seem to have this one,
so it is now included only when asked for. */

#ifdef NEED_MEMMOVE

void memmove(char *s1, char *s2, int size)
{
char *temp = malloc(size);
memcpy(temp, s2, size);
memcpy(s1, temp, size);
free(temp);
}

#endif


/* End of elocal.sunos4.c */
