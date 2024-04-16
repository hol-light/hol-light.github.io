/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1992, 1994 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: March 1994 */

/* This file is specific to the support of OSF1 */

#include "sys/fcntl.h"
#include "sys/ioctl.h"
#include "termios.h"

/* Ensure the local_init1 function is called */

#define call_local_init1

/* Termcap functions */

extern int tgetent(char *, char *);
extern int tgetflag(char *);
extern int tgetnum(char *);
extern char *tgoto(char *, int, int);

#define NO_TERM_H

/* End of osf1local.h */
