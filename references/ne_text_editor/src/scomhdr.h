/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1993 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: June 1993 */

/* This header file is the interface between the "common" screen handling
functions and the system-dependent screen driver. */


/* Format of one entry in the screen buffer */

typedef struct {
  char ch;
  char rend;
} sc_buffstr;

/* Shared variables */

extern sc_buffstr *sc_buffer;            /* the screen buffer */

extern int sc_maxcol;                    /* maximum column */
extern int sc_maxrow;                    /* maximum row */


/*  Procedures */

extern void scommon_select(void);

extern void  (*sys_w_cls)(int, int, int, int);
extern void  (*sys_w_flush)(void);
extern void  (*sys_w_move)(int, int);
extern void  (*sys_w_rendition)(int);
extern void  (*sys_w_putc)(int);
extern void  (*sys_w_hscroll)(int, int, int, int, int);
extern void  (*sys_w_vscroll)(int, int, int, BOOL);


/* End of scomhdr.h */
