/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1994 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified November 1999 */


/* This file is specific to the support modules for RISC OS */
/* We don't need the xs interface in scrw for E, as all store
allocation is done by malloc. */

#define NO_XS

#include "kernel.h"
#include "wimphdr.h"

#ifndef NO_XS
#include "xs.h"
#endif

#include "scrw.h"

/* Some friendlier names */

#define SWIregs     _kernel_swi_regs
#define OSerror     _kernel_oserror

#define char_height  36
#define char_width   16

#define DragASprite_Start    0x42400
#define DragASprite_Stop     0x42401

#define Hourglass_On         0x406C0
#define Hourglass_Off        0x406C1

#define TaskWindow_Taskinfo  0x43380

#define OS_Byte              0x06
#define OS_File              0x08
#define OS_ReadModeVariable  0x35
#define OS_ReadVarVal        0x23
#define OS_SpriteOp          0x2e
#define OS_Word              0x07
#define OS_WriteC            0x00
#define OS_WriteN            0x46


#define icon_version        3
#define icon_cmdline        0

#define icon_line           0
#define icon_column         1

#define icon_query          1
#define icon_discard        0
#define icon_cancel         2
#define icon_save           3

#define icon_saveOK         0
#define icon_savename       1
#define icon_saveicon       2
#define icon_savetype       4

#define menu_txt_save       1
#define menu_txt_foreground 2
#define menu_txt_background 3



/*************************************************
*                 XS Functions                   *
*************************************************/

extern void   xs_free_all(void);
extern void  *xs_get(int);
extern void   xs_init(void);


/*************************************************
*               Global variables                 *
*************************************************/

extern short int ControlKeystroke[];
extern int hourglass_on;
extern int mode_max_col;
extern int mode_max_row;

extern SWIregs swi_regs;
extern int support_taskwindows;

extern int  SWI(int, int, ...);
extern OSerror *ESWI(int, int, ...);

/* End of roshdr.h */
