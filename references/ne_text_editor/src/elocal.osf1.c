/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1992, 1993 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: December 1993 */

/* This file is specific to the support of osf1. It contains
functions that are specific to the particular operating system,
or compiler, or whatever. */

#include "ehdr.h"

#include <sys/sysinfo.h>
#include <sys/proc.h>

void local_init1(void)
{
int buff[2];
buff[0] = SSIN_UACPROC;
buff[1] = UAC_NOPRINT;
setsysinfo(SSI_NVPAIRS, buff, 1, 0, 0);
}

/* End of elocal.osf1.c */
