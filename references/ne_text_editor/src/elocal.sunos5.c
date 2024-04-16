/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1992, 1993 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: December 1993 */

/* This file is specific to the support of SunOS. It contains
functions that are specific to the particular operating system,
or compiler, or whatever. */

#include "ehdr.h"


/* Under SunOS we are currently using the gnu compiler and the
normal libraries. They don't seem to have strerror(). */

char *strerror(int n)
{
if (n >= sys_nerr) return "unknown error number";
return sys_errlist[n];
}

/* End of elocal.sunos5.c */
