/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Written by Richard Brooksby, starting June 1997, for FreeBSD */
/* This file last modified for NetBSD: July 1999 by PH (after Ben Harris) */

/* This file is specific to the support of NetBSD */

#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <termcap.h>
#include <termios.h>

#define TERM_H <termcap.h>

/* End of netbsdlocal.h */
