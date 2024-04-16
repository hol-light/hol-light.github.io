/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1994 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: October 1994 */


/* This file contains screen-handling code for RISC OS, apart from the
wimp window-handling code, which is in a separate module. */


#include "ehdr.h"
#include "shdr.h"
#include "scomhdr.h"
#include "roshdr.h"
#include "kernel.h"
#include "keyhdr.h"

#define sh  s_f_shiftbit
#define ct  s_f_ctrlbit
#define shc (s_f_shiftbit + s_f_ctrlbit)

#define osbyte(a,b,c) _kernel_osbyte(a,b,c)
#define oswrch(c)     _kernel_oswrch(c)
#define osrdch()      _kernel_osrdch()



/*************************************************
*                 Data tables                    *
*************************************************/

/* Table for non-printing characters */

static char CtrlTable[] = {
68,116,92,68,17,17,17,14,48,64,32,25,105,15,9,9,
48,64,32,17,106,4,10,17,120,64,112,81,122,4,10,17,
120,64,112,64,127,4,4,4,120,64,112,70,121,9,11,7,
48,72,120,73,74,12,10,9,112,72,112,72,116,4,4,7,
112,72,112,75,116,2,1,6,72,72,120,72,95,4,4,4,
64,64,64,79,120,14,8,8,68,68,68,40,31,4,4,4,
120,64,112,79,72,14,8,8,56,64,64,78,57,14,10,9,
48,64,32,22,105,9,9,6,48,64,32,23,98,2,2,7,
112,72,72,72,120,8,8,15,112,72,72,74,118,2,2,7,
112,72,72,78,121,2,4,15,112,72,72,78,121,6,1,14,
112,72,72,74,118,10,31,2,72,104,88,73,74,12,10,9,
48,64,32,17,106,4,4,4,120,64,112,78,121,14,9,14,
56,64,64,64,57,13,11,9,120,64,112,64,123,21,21,17,
56,64,48,14,121,14,9,14,120,64,112,71,120,8,8,7,
120,64,112,71,72,6,1,14,56,64,88,79,56,6,1,14,
112,72,112,75,76,2,1,6,72,72,72,75,52,2,1,6};


/* Control keystrokes to return for keys that are not
dealt with locally nor are ctrls. This table is consulted
from wros to ignore undefined keys. The data supplied to
sys_keystroke actually has 0x100 added, because this is the
way values are generated in the Wimp - leaving 0x80 to 0xFF
for data. */

short int ControlKeystroke[] = {
                                      s_f_del,         /* 7F    */
  s_f_umax+10,s_f_umax+1, s_f_umax+2, s_f_umax+3,      /* 80-83 */
  s_f_umax+4, s_f_umax+5, s_f_umax+6, s_f_umax+7,      /* 84-87 */
  s_f_umax+8, s_f_umax+9, s_f_tab,    s_f_copykey,     /* 88-8B */
  s_f_clf,    s_f_crt,    s_f_cdn,    s_f_cup,         /* 8C-8F */
  s_f_umax+20,s_f_umax+11,s_f_umax+12,s_f_umax+13,     /* 90-93 */
  s_f_umax+14,s_f_umax+15,s_f_umax+16,s_f_umax+17,     /* 94-97 */
  s_f_umax+18,s_f_umax+19,s_f_tab+sh, s_f_copykey+sh,  /* 98-9B */
  s_f_clf+sh, s_f_crt+sh, s_f_cdn+sh, s_f_cup+sh,      /* 9C-9F */
  s_f_umax+30,s_f_umax+21,s_f_umax+22,s_f_umax+23,     /* A0-A3 */
  s_f_umax+24,s_f_umax+25,s_f_umax+26,s_f_umax+27,     /* A4-A7 */
  s_f_umax+28,s_f_umax+29,s_f_tab+ct, s_f_copykey+ct,  /* A8-AB */
  s_f_clf+ct, s_f_crt+ct, s_f_cdn+ct, s_f_cup+ct,      /* AC-AF */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,        /* B0-B3 */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,        /* B4-B7 */
  s_f_reshow, s_f_user,   s_f_tab+shc,s_f_copykey+shc, /* B8-BB */
  s_f_clf+shc,s_f_crt+shc,s_f_cdn+shc,s_f_cup+shc,     /* BC-BF */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,        /* C0-C3 */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,        /* C4-C7 */
  s_f_user,   s_f_user,   s_f_umax+10,s_f_user,        /* C8-CB */
  s_f_user,   s_f_ins,    s_f_user,   s_f_user,        /* CC-CF */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,        /* D0-D3 */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,        /* D4-D7 */
  s_f_user,   s_f_user,   s_f_umax+20,s_f_user,        /* D8-DB */
  s_f_user,   s_f_ins+sh, s_f_user,   s_f_user,        /* DC-DF */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,        /* E0-E3 */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,        /* E4-E7 */
  s_f_user,   s_f_user,   s_f_umax+30,s_f_user,        /* E8-EB */
  s_f_user,   s_f_ins+ct, s_f_user,   s_f_user,        /* EC-EF */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,        /* F0-F3 */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,        /* F4-F7 */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,        /* F8-FB */
  s_f_user,   s_f_ins+shc,s_f_user,   s_f_user,        /* FC-FF */

/* Fudge values for handling backspace as separate from ctrl/H and for
handling shift+ctrl with delete. */

  s_f_del+sh, s_f_del+ct, s_f_del+shc,                 /* del with shift/ctrl */
  s_f_bsp,    s_f_bsp+sh, s_f_bsp+ct, s_f_bsp+shc };   /* backspace */



/*************************************************
*             Static Data                        *
*************************************************/

static int sros_setrendition = s_r_normal;
static int sros_datanext = FALSE;



/*************************************************
*              Accept keystroke                  *
*************************************************/

/* This procedure is used both in window and non-window operation. It is
given a raw keystroke, and must call the appropriate handling procedure.
Shift/ctrl/copy (0x1BB) is used to signal that a data keystroke follows. */

void sys_keystroke(int key)
{
if (key == 0x1BB && !sros_datanext) sros_datanext = TRUE;

else if (setjmp(error_jmpbuf) == 0)
  {
  BOOL flush = TRUE;
  error_longjmpOK = TRUE;

  if (key < 32 && !sros_datanext) key_handle_function(key);
    else if (key >= 0x17F && !sros_datanext)
      key_handle_function(ControlKeystroke[key-0x17F]);
  else
    {
    key_handle_data(key);
    ESWI(6, 2, 152, 0);
    flush = (swi_regs.r[2] == key);
    sros_datanext = FALSE;
    }

  if (!main_done && flush) ESWI(6, 2, 15, 1);
  }
error_longjmpOK = FALSE;
}


/*************************************************
*       Get keystroke for command line           *
*************************************************/

/* This function is called when reading a command line in screen, but
non-wimp, mode. It must return a type and a value. */

int sys_cmdkeystroke(int *type)
{
int key = osrdch();
if (key == 8 && osbyte(121, 0xaf, 0) != 0) key = 0x103;
if (key == 127 || key == 0x103)          /* fudges are required for backspace and delete */
  {
  int shift = osbyte(121, 0x80, 0)? 1 : 0;
  int ctrl = osbyte(121, 0x81, 0)? 2 : 0;
  if (key == 0x103) key += shift + ctrl;                      /* 0x103 - 0x106 */
    else if (shift + ctrl != 0) key = 0xff + shift + ctrl;  /* 0x0ff, 0x100 - 0x102 */
  }
if (key < 32) *type = ktype_ctrl;
  else if (key >= 127)
    {
    *type = ktype_function;
    key = ControlKeystroke[key-127];
    }
  else *type = ktype_data;
return key;
}



/*************************************************
*         Interface routines to scommon          *
*     For screen, but non-wimp, processing       *
*************************************************/


static void sros_cls(int bottom, int left, int top, int right)
{
if (bottom != screen_max_row || left != 0 || top != 0 || right != screen_max_col)
  {
  oswrch(28); oswrch(left); oswrch(bottom); oswrch(right); oswrch(top);
  }
oswrch(12);
oswrch(26);
}

static void sros_move(int x, int y)
{
oswrch(31); oswrch(x); oswrch(y);
}

static void sros_rendition(int r)
{
if (r != sros_setrendition)
  {
  int i;
  oswrch(23); oswrch(17); oswrch(5);
  for (i = 0; i < 7; i++) oswrch(0);
  sros_setrendition = r;
  }
}

static void sros_putc(int c)
{
if (c < 32 || (c >= 127 && c < topbit_minimum) || (!main_eightbit && c >= topbit_minimum))
  {
  int i;
  oswrch(23); oswrch(129);
  if (c >= 128) c = 26;   /* SUB */
  if (c == 127) for (i = 1; i <= 8; i++) oswrch(85);
    else for (i = c*8; i <= c*8 + 7; i++) oswrch(CtrlTable[i]);
  oswrch(129);
  }
else oswrch(c);
}


static void sros_hscroll(int left, int bottom, int right, int top, int amount)
{
int i;
oswrch(28); oswrch(left); oswrch(bottom); oswrch(right); oswrch(top);
for (i = 0; i < abs(amount); i++)
  {
  int j;
  oswrch(23); oswrch(7); oswrch(0); oswrch((amount > 0)? 0:1);
  for (j = 0; j < 7; j++) oswrch(0);
  }
oswrch(26);
}



static void sros_vscroll(int bottom, int top, int amount, BOOL deleting)
{
int i;
deleting = deleting;
oswrch(28); oswrch(0); oswrch(bottom); oswrch(screen_max_col); oswrch(top);
for (i = 0; i < abs(amount); i++)
  {
  int j;
  oswrch(23); oswrch(7); oswrch(0); oswrch((amount > 0)? 2:3);
  for (j = 0; j < 6; j++) oswrch(0);
  }
oswrch(26);
}




/*************************************************
*         Position to screen bottom for crash    *
*************************************************/

/* This function is called when about to issue a disaster error message.
It should position the cursor to a clear point at the screen bottom. */

void sys_crashposition(void)
{
sros_rendition(s_r_normal);
sros_move(0, screen_max_row);
main_screenmode = main_screenOK = FALSE;
}



/*************************************************
*              Flush any buffering               *
*************************************************/

static void sros_flush(void)
{
}


/*************************************************
*       Entry point for non-window screen run    *
*************************************************/

void sys_runscreen(void)
{
FILE *fid = NULL;
int filetype = default_filetype;
char *fromname = arg_from_name;
char *toname = (arg_to_name == NULL)? arg_from_name : arg_to_name;

sys_w_cls = sros_cls;
sys_w_flush = sros_flush;
sys_w_move = sros_move;
sys_w_rendition = sros_rendition;
sys_w_putc = sros_putc;
sys_w_hscroll = sros_hscroll;
sys_w_vscroll = sros_vscroll;

/* No need to set/reset for command-line styles */

sys_setupterminal = NULL;
sys_resetterminal = NULL;

arg_from_name = arg_to_name = NULL;
currentbuffer = main_bufferchain = NULL;

/* If a file name is given, check for its existence before setting up
the screen. */

if (fromname != NULL && strcmp(fromname, "-") != 0)
  {
  fid = sys_fopen(fromname, "r", &filetype);
  if (fid == NULL)
    {
    printf("** The file \"%s\" could not be opened.\n", fromname);
    printf("** NE abandoned.\n");
    main_rc = 16;
    return;
    }
  }

/* Set up the screen and cause windows to be defined */

s_init(screen_max_row, screen_max_col, TRUE);
scrn_init(TRUE);
scrn_windows();
main_screenOK = TRUE;
default_rmargin = main_rmargin;   /* The initial one = window_width */

if (init_init(fid, fromname, filetype, toname, main_stream))
  {
  int savecurstate = osbyte(237,0,255);
  int savetab = osbyte(219,0,255);
  int savefkeys[8];
  int i, x, y;
  for (i = 0; i <= 7; i++) savefkeys[i] = osbyte(221+i, 0, 255);

  if (!main_noinit && main_einit != NULL) cmd_obey(main_einit);
  main_initialized = TRUE;

  osbyte(219,0x8A,0);
  osbyte(221,0xC0,0);
  osbyte(222,0xD0,0);
  osbyte(223,0xE0,0);
  osbyte(224,0xF0,0);
  osbyte(225,0x80,0);
  osbyte(226,0x90,0);
  osbyte(227,0xA0,0);
  osbyte(228,0xB0,0);
  osbyte(237,2,0);

  if (main_opt != NULL)
    {
    int yield;
    scrn_display(FALSE);
    s_selwindow(message_window, 0, 0);
    s_rendition(s_r_normal);
    s_flush();
    yield = cmd_obey(main_opt);
    if (yield != done_continue && yield != done_finish)
      {
      screen_forcecls = TRUE;
      scrn_rdline(TRUE, FALSE, "Press RETURN to continue ");
      }
    }

  if (!main_done)
    {
    scrn_display(FALSE);
    x = s_x();
    y = s_y();
    s_selwindow(message_window, 0, 0);
    if (screen_max_col > 36) 
      s_printf("NE version %s %s", version_string, version_date);
    if (key_table[7] == ka_rc && screen_max_col > 64)
      s_printf("  To exit, press ctrl/G, W, Return.");
    main_shownlogo = TRUE;
    s_selwindow(first_window, x, y);
    }

  while (!main_done)
    {
    int k;
    main_rc = error_count = 0;           /* reset at each interaction */
    k = osrdch();
    if (k == 8 && osbyte(121, 0xaf, 0) != 0) k = 0x103;
    if (k == 127 || k == 0x103)          /* fudges are required for backspace and delete */
      {
      int shift = osbyte(121, 0x80, 0)? 1 : 0;
      int ctrl = osbyte(121, 0x81, 0)? 2 : 0;
      if (k == 0x103) k += shift + ctrl;                      /* 0x103 - 0x106 */
        else if (shift + ctrl != 0) k = 0xff + shift + ctrl;  /* 0x07f, 0x100 - 0x102 */
      }

    /* Function keys and things have values >= 0x80, and we have created some
    extras > 0xFF. However, from the Wimp there are data values up to 0xFF, and
    the functions etc. have &100 added, so we make these compatible. */

    if (k >= 0x7F) k += 0x100;
    sys_keystroke(k);
    main_flush_interrupt();
    }

  sros_rendition(s_r_normal);
  sros_move(0, screen_max_row);
  for (i = 0; i <= 7; i++) osbyte(221+i, savefkeys[i], 0);
  osbyte(219, savetab, 0);
  osbyte(237, savecurstate, 0);
  }
}

/* End of sros.c */
