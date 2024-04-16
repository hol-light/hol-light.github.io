/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1992, 1994 */

/* Written by Philip Hazel, starting November 1991 */

/* This file is specific to the support of BSDI */

#include "sys/fcntl.h"
#include "termios.h"
#include "sys/filio.h"

extern char *tgoto(char *, int, int);

#define NO_TERM_H

/* End of linuxlocal.h */
