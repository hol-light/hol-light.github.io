/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1992, 1993, 1994 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: June 1994 */

/* This file is specific to the support of HP-UX. It contains
functions that are specific to the particular operating system,
or compiler, or whatever. */

#include "ehdr.h"

char *strerror(int n)
{
if (n >= sys_nerr) return "unknown error number";
return sys_errlist[n];
}

/* End of elocal.hpux.c */
