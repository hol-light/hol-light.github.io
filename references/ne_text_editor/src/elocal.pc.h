/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1993    *
 *                                                            *
 * Written by Philip Hazel, starting November 1991            *
 * Adapted for the PC by Brian Holley, starting November 1993 *
 * This file last modified: January 1994                      *
 *                                                            *
 * The PC version uses Borland C++ (C code only) version 3.1  *
 *                                                            *
 * This file contains the system-specific header information  *
 * for the PC                                                 */

#define HELP_FILE_NAME "ne.hlp"
#define PATH_SEPARATOR '\\'
#define PATH_DIR_SEPARATOR ';'
#define SIGHUP SIGABRT
#ifndef __WIN32CON__
#define store_allocation_unit 16*1024
#define store_allocation_unit_is_max
#endif

