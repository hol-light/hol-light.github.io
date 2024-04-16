/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1992, 1993 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: June 1993 */

/* This file is specific to the support of SunOS 4. It contains
things that have to get imported into the main ehdr.h file
for system-specific reasons. */


#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1


extern char *strerror(int);
extern int sys_nerr;
extern char *sys_errlist[];

/* End of elocal.sunos4.h */
