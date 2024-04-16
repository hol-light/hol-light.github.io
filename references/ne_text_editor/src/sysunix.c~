/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 2003 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: March 2003 */


/* This file contains the system-specific routines for Unix,
with the exception of the window-specific code, which lives in
its own modules. */

#include <memory.h>
#include <fcntl.h>
#include <pwd.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
            
#include "ehdr.h"
#include "unixhdr.h"
#include "scomhdr.h"
#include "keyhdr.h"

#define this_version "0.18"

#define tc_keylistsize 1024


/* List of signals to be trapped for buffer dumping on
crashes to be effective. Names are for use in messages. */

/* SIGHUP is handled specially, so does not appear here - otherwise
problems with dedicated xterm windows. */

/* SIGXFSZ and SIGXCPU not on HP-UX */
/* SIGSYS not on Linux */

int signal_list[] = {
  SIGQUIT, SIGILL,  SIGIOT,
  SIGFPE,  SIGBUS,  SIGSEGV, SIGTERM,
#ifdef SIGSYS
  SIGSYS,
#endif
#ifdef SIGXCPU
  SIGXCPU,
#endif
#ifdef SIGXFSZ
  SIGXFSZ,
#endif
  -1 };

char *signal_names[] = {
  "(SIGQUIT)", "(SIGILL)",  "(SIGIOT)",
  "(SIGFPE)",  "(SIGBUS)",  "(SIGSEGV)", "(SIGTERM)",
#ifdef SIGSYS
  "(SIGSYS)",
#endif
#ifdef SIGXCPU
  "(SIGXCPU)",
#endif
#ifdef SIGXFSZ
  "(SIGXFSZ)",
#endif
  "" };



/*************************************************
*               Static data                      *
*************************************************/

#ifdef HAVE_TERMCAP
static char *termcap_buf = NULL;
#endif

static char *term_name;
static int term_type;


/* The following keystrokes are not available and so their default
settings must be squashed. Note that we leave "ctrl/tab", because
esc tab maps to it and is suitable translated for output. For some
known terminals (e.g. Archimedes ttp) some of these keys are not
squashed, because they are built into this code. There are special
versions of the table for these. */

#define s s_f_shiftbit
#define c s_f_ctrlbit
#define sc s_f_shiftbit+s_f_ctrlbit

static unsigned char non_keys[] = {
  s_f_cup+s,   s_f_cup+c,     s_f_cup+sc,
  s_f_cdn+s,   s_f_cdn+c,     s_f_cdn+sc,
  s_f_clf+s,   s_f_clf+c,     s_f_clf+sc,
  s_f_crt+s,   s_f_crt+c,     s_f_crt+sc,
  s_f_copykey, s_f_copykey+s, s_f_copykey+c,
  0 };

/* This does in fact require some special configuration of xterm... */

static unsigned char xterm_non_keys[] = {
  s_f_cup+sc,
  s_f_cdn+sc,
  s_f_clf+sc,
  s_f_crt+sc,
  s_f_copykey, s_f_copykey+s, s_f_copykey+c,
  0 };

static unsigned char ttpa_non_keys[] = {
  s_f_cup+sc,
  s_f_cdn+sc,
  s_f_clf+sc,
  s_f_crt+sc,
  0 };

static unsigned char fawn_non_keys[] = {
  s_f_cup+c,   s_f_cup+sc,
  s_f_cdn+c,   s_f_cdn+sc,
  s_f_clf+c,   s_f_clf+sc,
  s_f_crt+c,   s_f_crt+sc,
  s_f_copykey, s_f_copykey+s, s_f_copykey+c,
  0 };

#undef s
#undef c
#undef sc




/*************************************************
*          Add to arg string if needed           *
*************************************************/

/* This function permits the system-specific code to add additional
argument definitions to the arg string which will be used to decode
the command line. */

char *sys_argstring(char *s)
{
return s;
}


/*************************************************
*      Deal with local argument settings         *
*************************************************/

/* This procedure is called when the command line has been decoded,
to enable the system-specific code to deal with its own argument
settings. The first argument points to the rdargs results structure,
while the second is the number of the first local argument. This is
called before the system-independent arguments are examined. */

void sys_takeargs(arg_result *result, int n)
{
}



/*************************************************
*     Read a termcap or terminfo string entry    *
*************************************************/

static char *my_tgetstr(char *key)
{
#ifdef HAVE_TERMCAP
char *yield, *p;
char workbuff[256];
p = workbuff;
if (tgetstr(key, &p) == 0 || workbuff[0] == 0) return NULL;
yield = (char *)malloc(strlen(workbuff)+1);
strcpy(yield, workbuff);
return yield;

#else
char *yield = (char *)tigetstr(key);
if (yield == NULL || yield == (char *)(-1) || yield[0] == 0) return NULL;
return yield;
#endif
}


/*************************************************
*      Add an escape key sequence to the list    *
*************************************************/

static void addkeystr(char *s, int keyvalue, int *acount, unsigned char **aptr)
{
unsigned char *ptr = *aptr;
if (s[1] == 0) tc_k_trigger[(unsigned char)(s[0])] = keyvalue; else
  {
  tc_k_trigger[(unsigned char)(s[0])] = 254;
  *acount += 1;
  *ptr = strlen(s) + 3;
  strcpy((char *)(ptr + 1), s);
  ptr[*ptr-1] = keyvalue;
  *aptr = ptr + *ptr;
  }
}


/*************************************************
* Read a termcap/info key entry and set up data  *
*************************************************/

static int tgetkeystr(char *s, int *acount, unsigned char **aptr, int keyvalue)
{
#ifdef HAVE_TERMCAP
char workbuff[256];
char *p = workbuff;
if (tgetstr(s, &p) == 0 || workbuff[0] == 0) return 0;

#else
char *workbuff = (char *)tigetstr(s);
if (workbuff == NULL || workbuff == (char *)(-1) || workbuff[0] == 0) return 0;
#endif

addkeystr(workbuff, keyvalue, acount, aptr);
return 1;   /* non-zero means OK */
}



/*************************************************
*            Check terminal type                 *
*************************************************/

/* Search the termcap database. If the terminal has sufficient
power, we can remain in screen mode; otherwise return a non-
screen terminal type.

Alternatively, search the terminfo database, depending on a
configuration option. */

/* Define alternative names for string functions */

#ifdef HAVE_TERMCAP
#define AL   "al"
#define BC   "bc"
#define CD   "ce"
#define CL   "cl"
#define CM   "cm"
#define CS   "cs"
#define DC   "dc"
#define DL   "dl"
#define DM   "dm"
#define IC   "ic"
#define IM   "im"
#define IP   "ip"
#define KE   "ke"
#define KS   "ks"
#define PC   "pc"
#define SE   "se"
#define SF   "sf"
#define SO   "so"
#define SR   "sr"
#define TE   "te"
#define TI   "ti"
#define UP   "up"

#define KU   "ku"
#define KD   "kd"
#define KL   "kl"
#define KR   "kr"

#define K0   "k0"
#define K1   "k1"
#define K2   "k2"
#define K3   "k3"
#define K4   "k4"
#define K5   "k5"
#define K6   "k6"
#define K7   "k7"
#define K8   "k8"
#define K9   "k9"
#define KK   "k;"
#define F1   "F1"
#define F2   "F2"
#define F3   "F3"
#define F4   "F4"
#define F5   "F5"
#define F6   "F6"
#define F7   "F7"
#define F8   "F8"
#define F9   "F9"
#define FA   "FA"
#define FB   "FB"
#define FC   "FC"
#define FD   "FD"
#define FE   "FE"
#define FF   "FF"
#define FG   "FG"
#define FH   "FH"
#define FI   "FI"
#define FJ   "FJ"
#define FK   "FK"

#else
#define AL   "il1"
#define BC   "cub1"
#define CD   "ed"
#define CL   "clear"
#define CM   "cup"
#define CS   "csr"
#define DC   "dch1"
#define DL   "dl1"
#define DM   "smdc"
#define IC   "ich1"
#define IM   "smir"
#define IP   "ip"
#define KE   "rmkx"
#define KS   "smkx"
#define PC   "pad"
#define SE   "rmso"
#define SF   "ind"
#define SO   "smso"
#define SR   "ri"
#define TE   "rmcup"
#define TI   "smcup"
#define UP   "cuu1"

#define KU   "kcuu1"
#define KD   "kcud1"
#define KL   "kcub1"
#define KR   "kcuf1"

#define K0   "kf0"
#define K1   "kf1"
#define K2   "kf2"
#define K3   "kf3"
#define K4   "kf4"
#define K5   "kf5"
#define K6   "kf6"
#define K7   "kf7"
#define K8   "kf8"
#define K9   "kf9"
#define KK   "kf10"
#define F1   "kf11"
#define F2   "kf12"
#define F3   "kf13"
#define F4   "kf14"
#define F5   "kf15"
#define F6   "kf16"
#define F7   "kf17"
#define F8   "kf18"
#define F9   "kf19"
#define FA   "kf20"
#define FB   "kf21"
#define FC   "kf22"
#define FD   "kf23"
#define FE   "kf24"
#define FF   "kf25"
#define FG   "kf26"
#define FH   "kf27"
#define FI   "kf28"
#define FJ   "kf29"
#define FK   "kf30"
#endif




static int CheckTerminal(void)
{
char *p;
unsigned char *keyptr;
int erret;
int keycount = 0;

/* Set up a file descriptor to the terminal for use in various
ioctl calls. */

ioctl_fd = open("/dev/tty", O_RDWR);

#ifdef HAVE_TERMCAP
if (termcap_buf == NULL) termcap_buf = (char *)malloc(1024);
if (tgetent(termcap_buf, term_name) != 1) return term_other;
#else
if (setupterm(term_name, ioctl_fd, &erret) != OK || erret != 1)
  return term_other;
#endif

/* First, investigate terminal size. See if we can get the values
from an ioctl call. If not, we take them from termcap/info . */

tc_n_li = 0;
tc_n_co = 0;

#ifdef unixwinsz
  {
  struct winsize parm;
  if (ioctl(ioctl_fd, TIOCGWINSZ, &parm) == 0)
    {
    if (parm.ws_row != 0) tc_n_li = parm.ws_row;
    if (parm.ws_col != 0) tc_n_co = parm.ws_col;
    }
  }
#endif


#ifdef HAVE_TERMCAP
if (tc_n_li == 0) tc_n_li = tgetnum("li");  /* number of lines on screen */
if (tc_n_co == 0) tc_n_co = tgetnum("co");  /* number of columns on screen */
#else
if (tc_n_li == 0) tc_n_li = tigetnum("lines");  /* number of lines on screen */
if (tc_n_co == 0) tc_n_co = tigetnum("cols");  /* number of columns on screen */
#endif

if (tc_n_li <= 0 || tc_n_co <= 0) return term_other;

/* Terminal must be capable of moving the cursor to arbitrary positions */

if ((p = tc_s_cm = my_tgetstr(CM)) == 0) return term_other;

/* Have a look at the "cm" string. If it contains the characters
"%." ("%c" for TERMINFO) then cursor positions are required to be
output in binary. Set the flag to cause the positioning routines
never to generate a move to row or column zero, since binary zero is
used to mark the end of the string. (Also various bits of
communications tend to eat binary zeros.) */

NoZero = FALSE;
while (*p)
  {
#ifdef HAVE_TERMCAP
  if (*p == '%' && p[1] == '.') NoZero = TRUE;
#else
  if (*p == '%' && p[1] == 'c') NoZero = TRUE;
#endif
  p++;
  }

/* If NoZero is set, we need cursor up and cursor left movements;
they are not otherwise used. If the "bc" element is not set, cursor
left is backspace. */

if (NoZero)
  {
  if ((tc_s_up = my_tgetstr(UP)) == 0) return term_other;
  tc_s_bc = my_tgetstr(BC);
  }

/* Set the automatic margins flag */

#ifdef HAVE_TERMCAP
tc_f_am = tgetflag("am");
#else
tc_f_am = tigetflag("am");
#endif

/* Some facilities are optional - E will use them if present,
but will use alternatives if they are not. */

tc_s_al = my_tgetstr(AL);  /* add line */
tc_s_ce = my_tgetstr(CD);  /* clear EOL */
tc_s_cl = my_tgetstr(CL);  /* clear screen */
tc_s_cs = my_tgetstr(CS);  /* set scroll region */
tc_s_dl = my_tgetstr(DL);  /* delete line */
tc_s_ip = my_tgetstr(IP);  /* insert padding */
tc_s_ke = my_tgetstr(KE);  /* unsetup keypad */
tc_s_ks = my_tgetstr(KS);  /* setup keypad */
tc_s_pc = my_tgetstr(PC);  /* pad char */
tc_s_se = my_tgetstr(SE);  /* end standout */
tc_s_sf = my_tgetstr(SF);  /* scroll up */
tc_s_so = my_tgetstr(SO);  /* start standout */
tc_s_sr = my_tgetstr(SR);  /* scroll down */
tc_s_te = my_tgetstr(TE);  /* end screen management */
tc_s_ti = my_tgetstr(TI);  /* init screen management */

/* E requires either "set scroll region" with "scroll down", or "delete line"
with "add line" in order to implement scrolling. */

if ((tc_s_cs == NULL || tc_s_sr == NULL) &&
  (tc_s_dl == NULL || tc_s_al == NULL))
    return term_other;

/* If no highlighting, set to null strings */

if (tc_s_se == NULL || tc_s_so == NULL) tc_s_se = tc_s_so = "";

/* E is prepared to use "insert char" provided it does not have to
switch into an "insert mode", and similarly for "delete char". */

if ((tc_s_ic = my_tgetstr(IC)) != NULL)
  {
  char *tv = my_tgetstr(IM);
  if (tc_s_ic == NULL || (tv != NULL && *tv != 0)) tc_s_ic = NULL;
#ifdef HAVE_TERMCAP
  if (tv != NULL) free(tv);
#endif
  }

if ((tc_s_dc = my_tgetstr(DC)) != NULL)
  {
  char *tv = my_tgetstr(DM);
  if (tc_s_dc == NULL || (tv != NULL && *tv != 0)) tc_s_dc = NULL;
#ifdef HAVE_TERMCAP
  if (tv != NULL) free(tv);
#endif
  }

/* Now we must scan for the strings sent by special keys and
construct a data structure for sunix to scan. Only the
cursor keys are mandatory. */

tc_k_strings = (unsigned char *)malloc(tc_keylistsize);
keyptr = tc_k_strings + 1;

tc_k_trigger = (unsigned char *)malloc(256);
memset((void *)tc_k_trigger, 255, 256);  /* all unset */

if (tgetkeystr(KU, &keycount, &keyptr, Pkey_up) == 0) return term_other;
if (tgetkeystr(KD, &keycount, &keyptr, Pkey_down) == 0) return term_other;
if (tgetkeystr(KL, &keycount, &keyptr, Pkey_left) == 0) return term_other;
if (tgetkeystr(KR, &keycount, &keyptr, Pkey_right) == 0) return term_other;

/* Some terminals have more facilities than can be described by termcap/info.
Knowledge of some of them is screwed in to this code. If termcap/info ever
expands, this can be generalized. */

tt_special = tt_special_none;

if (strcmp(term_name, "ttpa") == 0 || strcmp(term_name, "arc-hydra") == 0)
  tt_special = tt_special_ttpa;
else if (strcmp(term_name, "termulator-bbca") == 0)
            tt_special = tt_special_ArcTermulator;
else if (strncmp(term_name, "xterm",5) == 0) tt_special = tt_special_xterm;
else if (strcmp(term_name, "fawn") == 0) tt_special = tt_special_fawn;

/* If there's screen management, and this is Xterm, we don't need a
newline at the end. */

if (tt_special == tt_special_xterm && tc_s_ti != NULL) main_nlexit = FALSE;

/* Could use some recent capabilities for some of these, but there
isn't a complete set defined yet, and the termcaps/terminfos don't
always have them in. */

if (tt_special == tt_special_fawn)
  {
  addkeystr("\033OP\033OD", Pkey_sh_left, &keycount, &keyptr);   /* #4 */
  addkeystr("\033OP\033OC", Pkey_sh_right, &keycount, &keyptr);  /* %i */
  addkeystr("\033OP\033OA", Pkey_sh_up, &keycount, &keyptr);
  addkeystr("\033OP\033OB", Pkey_sh_down, &keycount, &keyptr);
  }

if (tt_special == tt_special_xterm)
  {
  addkeystr("\033Ot", Pkey_sh_left, &keycount, &keyptr);   /* #4 */
  addkeystr("\033Ov", Pkey_sh_right, &keycount, &keyptr);  /* %i */
  addkeystr("\033Ox", Pkey_sh_up, &keycount, &keyptr);
  addkeystr("\033Or", Pkey_sh_down, &keycount, &keyptr);

  /* These allocations require suitable configuration of the xterm */
  addkeystr("\033OT", Pkey_ct_left, &keycount, &keyptr);
  addkeystr("\033OV", Pkey_ct_right, &keycount, &keyptr);
  addkeystr("\033OX", Pkey_ct_up, &keycount, &keyptr);
  addkeystr("\033OR", Pkey_ct_down, &keycount, &keyptr);
  addkeystr("\033OM", Pkey_ct_tab, &keycount, &keyptr);
  addkeystr("\033OP", Pkey_sh_del127, &keycount, &keyptr);
  addkeystr("\033ON", Pkey_ct_del127, &keycount, &keyptr);
  addkeystr("\033OQ", Pkey_sh_bsp, &keycount, &keyptr);
  addkeystr("\033OO", Pkey_ct_bsp, &keycount, &keyptr);
  }

if (tt_special == tt_special_ttpa)
  {
  addkeystr("\033F", Pkey_copy, &keycount, &keyptr);      /* &5 */
  addkeystr("\033k", Pkey_sh_copy, &keycount, &keyptr);
  addkeystr("\033K", Pkey_ct_copy, &keycount, &keyptr);
  addkeystr("\033l", Pkey_sh_left, &keycount, &keyptr);   /* #4 */
  addkeystr("\033m", Pkey_sh_right, &keycount, &keyptr);  /* %i */
  addkeystr("\033o", Pkey_sh_up, &keycount, &keyptr);
  addkeystr("\033n", Pkey_sh_down, &keycount, &keyptr);

  addkeystr("\033J", Pkey_ct_left, &keycount, &keyptr);
  addkeystr("\033I", Pkey_ct_right, &keycount, &keyptr);
  addkeystr("\033G", Pkey_ct_up, &keycount, &keyptr);
  addkeystr("\033H", Pkey_ct_down, &keycount, &keyptr);

  addkeystr("\033p", Pkey_sh_del127, &keycount, &keyptr);
  addkeystr("\033q", Pkey_ct_del127, &keycount, &keyptr);
  addkeystr("\033s", Pkey_bsp, &keycount, &keyptr);
  addkeystr("\033t", Pkey_sh_bsp, &keycount, &keyptr);
  addkeystr("\033u", Pkey_ct_bsp, &keycount, &keyptr);
  }

if (tt_special == tt_special_ArcTermulator)
  {
  addkeystr("\033[1!", Pkey_insert, &keycount, &keyptr);   /* insert key */
  addkeystr("\033[4!", Pkey_del127, &keycount, &keyptr);   /* delete key */
  addkeystr("\033[2!", Pkey_home, &keycount, &keyptr);     /* home key */
  addkeystr("\033[5!", Pkey_copy, &keycount, &keyptr);     /* copy key */
  addkeystr("\033[3!", Pkey_sh_up, &keycount, &keyptr);    /* page up */
  addkeystr("\033[6!", Pkey_sh_down, &keycount, &keyptr);  /* page down */
  addkeystr("\033[D",  Pkey_sh_left, &keycount, &keyptr);  /* shift left */
  addkeystr("\033[C",  Pkey_sh_right, &keycount, &keyptr); /* shift right */
  }

/* Termcap/info supported function keys */

tgetkeystr(K0, &keycount, &keyptr, Pkey_f0);
tgetkeystr(K1, &keycount, &keyptr, Pkey_f0+1);
tgetkeystr(K2, &keycount, &keyptr, Pkey_f0+2);
tgetkeystr(K3, &keycount, &keyptr, Pkey_f0+3);
tgetkeystr(K4, &keycount, &keyptr, Pkey_f0+4);
tgetkeystr(K5, &keycount, &keyptr, Pkey_f0+5);
tgetkeystr(K6, &keycount, &keyptr, Pkey_f0+6);
tgetkeystr(K7, &keycount, &keyptr, Pkey_f0+7);
tgetkeystr(K8, &keycount, &keyptr, Pkey_f0+8);
tgetkeystr(K9, &keycount, &keyptr, Pkey_f0+9);
tgetkeystr(KK, &keycount, &keyptr, Pkey_f0+10);
tgetkeystr(F1, &keycount, &keyptr, Pkey_f0+11);
tgetkeystr(F2, &keycount, &keyptr, Pkey_f0+12);
tgetkeystr(F3, &keycount, &keyptr, Pkey_f0+13);
tgetkeystr(F4, &keycount, &keyptr, Pkey_f0+14);
tgetkeystr(F5, &keycount, &keyptr, Pkey_f0+15);
tgetkeystr(F6, &keycount, &keyptr, Pkey_f0+16);
tgetkeystr(F7, &keycount, &keyptr, Pkey_f0+17);
tgetkeystr(F8, &keycount, &keyptr, Pkey_f0+18);
tgetkeystr(F9, &keycount, &keyptr, Pkey_f0+19);
tgetkeystr(FA, &keycount, &keyptr, Pkey_f0+20);
tgetkeystr(FB, &keycount, &keyptr, Pkey_f0+21);
tgetkeystr(FC, &keycount, &keyptr, Pkey_f0+22);
tgetkeystr(FD, &keycount, &keyptr, Pkey_f0+23);
tgetkeystr(FE, &keycount, &keyptr, Pkey_f0+24);
tgetkeystr(FF, &keycount, &keyptr, Pkey_f0+25);
tgetkeystr(FG, &keycount, &keyptr, Pkey_f0+26);
tgetkeystr(FH, &keycount, &keyptr, Pkey_f0+27);
tgetkeystr(FI, &keycount, &keyptr, Pkey_f0+28);
tgetkeystr(FJ, &keycount, &keyptr, Pkey_f0+29);
tgetkeystr(FK, &keycount, &keyptr, Pkey_f0+30);

/* Other terminals may support alternatives, or shifted versions,
or not use termcap/terminfo. */

if (tt_special == tt_special_ttpa)
  {
/*  addkeystr("\033X", Pkey_f0+10, &keycount, &keyptr)  // F10 */
/*  addkeystr("\033W", Pkey_f0+11, &keycount, &keyptr)  // F11 */
/*  addkeystr("\033V", Pkey_f0+12, &keycount, &keyptr)  // F12 */
  addkeystr("\033b", Pkey_f0+11, &keycount, &keyptr);  /* shift/f1 = f11 */
  addkeystr("\033c", Pkey_f0+12, &keycount, &keyptr);  /* shift/f2 = f12 */
  addkeystr("\033d", Pkey_f0+13, &keycount, &keyptr);  /* shift/f3 */
  addkeystr("\033e", Pkey_f0+14, &keycount, &keyptr);  /* shift/f4 */
  addkeystr("\033f", Pkey_f0+15, &keycount, &keyptr);  /* shift/f5 */
  addkeystr("\033g", Pkey_f0+16, &keycount, &keyptr);  /* shift/f6 */
  addkeystr("\033h", Pkey_f0+17, &keycount, &keyptr);  /* shift/f7 */
  addkeystr("\033i", Pkey_f0+18, &keycount, &keyptr);  /* shift/f8 */
  addkeystr("\033j", Pkey_f0+19, &keycount, &keyptr);  /* shift/f9 */
  addkeystr("\033.", Pkey_f0+20, &keycount, &keyptr);  /* shift/f10 */
  }

if (tt_special == tt_special_ArcTermulator)
  {
  addkeystr("\033[17!", Pkey_f0+1, &keycount, &keyptr);  /* function keys not*/
  addkeystr("\033[18!", Pkey_f0+2, &keycount, &keyptr);  /* in termcap/info */
  addkeystr("\033[19!", Pkey_f0+3, &keycount, &keyptr);
  addkeystr("\033[20!", Pkey_f0+4, &keycount, &keyptr);
  addkeystr("\033[21!", Pkey_f0+5, &keycount, &keyptr);
  addkeystr("\033[23!", Pkey_f0+6, &keycount, &keyptr);
  addkeystr("\033[24!", Pkey_f0+7, &keycount, &keyptr);
  addkeystr("\033[25!", Pkey_f0+8, &keycount, &keyptr);
  addkeystr("\033[26!", Pkey_f0+9, &keycount, &keyptr);
  addkeystr("\033[28!", Pkey_f0+10, &keycount, &keyptr);
  addkeystr("\033[29!", Pkey_f0+11, &keycount, &keyptr);
  addkeystr("\033[31!", Pkey_f0+12, &keycount, &keyptr);
  }

/* There are certain escape sequences that are built into NE. We
put them last so that they are only matched if those obtained
from termcap/terminfo do not contain the same sequences. */

addkeystr("\0330", Pkey_f0+10, &keycount, &keyptr);
addkeystr("\0331", Pkey_f0+1,  &keycount, &keyptr);
addkeystr("\0332", Pkey_f0+2,  &keycount, &keyptr);
addkeystr("\0333", Pkey_f0+3,  &keycount, &keyptr);
addkeystr("\0334", Pkey_f0+4,  &keycount, &keyptr);
addkeystr("\0335", Pkey_f0+5,  &keycount, &keyptr);
addkeystr("\0336", Pkey_f0+6,  &keycount, &keyptr);
addkeystr("\0337", Pkey_f0+7,  &keycount, &keyptr);
addkeystr("\0338", Pkey_f0+8,  &keycount, &keyptr);
addkeystr("\0339", Pkey_f0+9,  &keycount, &keyptr);

addkeystr("\033\0330", Pkey_f0+20, &keycount, &keyptr);
addkeystr("\033\0331", Pkey_f0+11, &keycount, &keyptr);
addkeystr("\033\0332", Pkey_f0+12, &keycount, &keyptr);
addkeystr("\033\0333", Pkey_f0+13, &keycount, &keyptr);
addkeystr("\033\0334", Pkey_f0+14, &keycount, &keyptr);
addkeystr("\033\0335", Pkey_f0+15, &keycount, &keyptr);
addkeystr("\033\0336", Pkey_f0+16, &keycount, &keyptr);
addkeystr("\033\0337", Pkey_f0+17, &keycount, &keyptr);
addkeystr("\033\0338", Pkey_f0+18, &keycount, &keyptr);
addkeystr("\033\0339", Pkey_f0+19, &keycount, &keyptr);

addkeystr("\033\033", Pkey_data, &keycount, &keyptr);
addkeystr("\033\177", Pkey_null, &keycount, &keyptr);
addkeystr("\033\015", Pkey_reshow, &keycount, &keyptr);
addkeystr("\033\t", Pkey_backtab, &keycount, &keyptr);
addkeystr("\033s", 19, &keycount, &keyptr);  /* ctrl/s */
addkeystr("\033q", 17, &keycount, &keyptr);  /* ctrl/q */

/* Set the count of strings in the first byte */

tc_k_strings[0] = keycount;

/* Remove the default actions for various shift+ctrl keys
that are not settable. This will prevent them from being
displayed. */

if (tt_special == tt_special_ttpa)
  {
  keycount = 0;
  while (ttpa_non_keys[keycount] != 0)
    key_table[ttpa_non_keys[keycount++]] = 0;
  }

else if (tt_special == tt_special_xterm)
  {
  keycount = 0;
  while (xterm_non_keys[keycount] != 0)
    key_table[xterm_non_keys[keycount++]] = 0;
  }

else if (tt_special == tt_special_fawn)
  {
  keycount = 0;
  while (fawn_non_keys[keycount] != 0)
    key_table[fawn_non_keys[keycount++]] = 0;
  }

else if (tt_special != tt_special_ttpa)
  {
  keycount = 0;
  while (non_keys[keycount] != 0) key_table[non_keys[keycount++]] = 0;
  }

/* Yield fullscreen terminal type */

return term_screen;
}


/*************************************************
*       Generate names for special keys          *
*************************************************/

/* This procedure is used to generate names for odd key
combinations that are used to represent keystrokes that
can't be generated normally. Return TRUE if the name
has been generated, else FALSE. */

BOOL sys_makespecialname(int key, char *buff)
{
if (key == s_f_tab + s_f_ctrlbit)
  {
  strcpy(buff, "esc tab    ");
  return TRUE;
  }
return FALSE;
}


/*************************************************
*         Additional special key text            *
*************************************************/

/* This function is called after outputting info about
special keys, to allow any system-specific comments to
be included. */

void sys_specialnotes(void)
{
error_printf("\n");
error_printf("esc-q        synonym for ctrl/q (XON)  esc-s          synonym for ctrl/s (XOFF)\n");
error_printf("esc-digit    functions 1-10            esc-esc-digit  functions 11-20\n");
error_printf("esc-return   re-display screen         esc-tab        backwards tab\n");
error_printf("esc-esc-char control char as data\n");

}


/*************************************************
*          Handle window size change             *
*************************************************/

void sigwinch_handler(int sig)
{
window_changed = TRUE;
signal(SIGWINCH, sigwinch_handler);
}



/*************************************************
*          Handle (ignore) SIGHUP                *
*************************************************/

/* SIGHUP's are usually ignored, as otherwise there are problems
with running NE in a dedicated xterm window. Ignored in the
sense of not trying to put out fancy messages, that is. 

December 1997: Not quite sure what the above comment is getting at,
but modified NE so as to do the dumping thing without trying to write 
anything to the terminal. Otherwise it was getting stuck if a connection
hung up. */

void sighup_handler(int sig)
{
crash_handler_chatty = FALSE;
crash_handler(sig);
}




/*************************************************
*              Local initialization              *
*************************************************/

/* This is called first thing in main() for doing vital system-
specific early things, especially those concerned with any
special store handling. */

void sys_init1(void)
{
signal(SIGHUP, sighup_handler);

#ifdef call_local_init1
local_init1();
#endif
main_tabs = getenv("NETABS");   /* Set default */
if (main_tabs == NULL) main_tabs = getenv("ETABS");   /* Set default */
}


/* This is called after argument decoding is complete to allow
any system-specific over-riding to take place. Main_screenmode
will be TRUE unless -line or -with was specified. */

void sys_init2(void)
{
int i;
char *filechars = "+-*/,.:!?";       /* leave only " and ' */

#ifdef call_local_init2
local_init2();
#endif

term_name = getenv("TERM");
main_einit = getenv("NEINIT");
if (main_einit == NULL) main_einit = getenv("EINIT");

/* Remove legal file characters from file delimiters list */

for (i = 0; i < (int)strlen(filechars); i++)
  ch_tab[(unsigned char)(filechars[i])] &= ~ch_filedelim;

/* Set up terminal type and terminal-specific things */

if (!main_screenmode) term_type = term_other; else
  {
  term_type = CheckTerminal();
  switch (term_type)
    {
    case term_screen:
    screen_max_row = tc_n_li - 1;
    screen_max_col = tc_n_co - 1;
    scommon_select();             /* connect the common screen driver */
    #ifdef unixwinsz
    signal(SIGWINCH, sigwinch_handler);
    #endif
    break;

    default:
    printf("This terminal (%s) cannot support screen editing in NE;\n",
      term_name);
    printf("therefore entering line mode:\n\n");
    main_screenmode = main_screenOK = FALSE;
    break;
    }
  }
}



/*************************************************
*                Munge return code               *
*************************************************/

/* This procedure is called just before exiting, to enable a
return code to be set according to the OS's conventions. */

int sys_rc(int rc)
{
return rc;
}


/*************************************************
*             Report local version               *
*************************************************/

/* This procedure is passed the global version string,
and should append to it the version of this local module,
together with the version(s) of any screen drivers. */

void sys_setversion(char *s)
{
strcat(s, "/");
strcat(s, this_version);
}


/*************************************************
*                   Beep                         *
*************************************************/

/* Called only when interactive. Makes the terminal beep.

Arguments:   none
Returns:     nothing
*/

void
sys_beep(void)
{
char buff[1];
int tty = open("/dev/tty", O_WRONLY);
buff[0] = 7;
write(tty, buff, 1);
close(tty);
}



/*************************************************
*        Decode ~ at start of file name          *
*************************************************/

/* Called both when opening a file, and when completing a file name.

Arguments:
  name         the current name
  len          length of same 
  buff         a buffer in which to return the expansion
  
Returns:       the buffer address if changed
               the name pointer if unchanged
*/

char *
sort_twiddle(char *name, int len, char *buff)
{
int i;
char logname[20];
struct passwd *pw;

/* For ~/thing, convert by reading the HOME variable */

if (name[1] == '/')
  {
  strcpy(buff, getenv("HOME"));
  strncat(buff, name+1, len-1);
  return buff; 
  }

/* Otherwise we must get the home directory from the
password file. */

for (i = 2;; i++) if (i >= len || name[i] == '/') break;
strncpy(logname, name+1, i-1);
logname[i-1] = 0;
pw = getpwnam(logname);
if (pw == NULL) strncpy(buff, name, len); else
  {
  strcpy(buff, pw->pw_dir);
  strncat(buff, name + i, len - i);
  } 
return buff; 
}



/*************************************************
*            Complete a file name                *
*************************************************/

/* This function is called when TAB is pressed in a command line while
interactively screen editing. It tries to complete the file name.

Arguments:
  cmd_buffer    the command buffer
  p             the current offset in the command buffer
  pmaxptr       pointer to the current high-water mark in the command buffer
  
Returns:        possibly updated value of p
                pmax may be updated via the pointer
*/

int
sys_fcomplete(char *cmd_buffer, int p, int *pmaxptr)
{
int pb = p - 1;
int pe = p;
int pmax = *pmaxptr;
int len, endlen;
char buffer[256];
char insert[256];
BOOL insert_found = FALSE;
BOOL beep = TRUE;
char *s;
DIR *dir;
struct dirent *dent;

if (p < 1 || cmd_buffer[pb] == ' ' || p > pmax) goto RETURN;
while (pb > 0 && cmd_buffer[pb - 1] != ' ') pb--;

/* One day we may implement completing in the middle of names, but for
the moment, this hack up just completes ends. */

/* while (pe < pmax && cmd_buffer[pe] != ' ') pe++; */
if (pe < pmax && cmd_buffer[pe] != ' ') goto RETURN;

len = pe - pb;

if (cmd_buffer[pb] == '~') 
  (void)sort_twiddle(cmd_buffer + pb, len, buffer);
else if (cmd_buffer[pb] != '/')
  {
  strcpy(buffer, "./");
  strncat(buffer, cmd_buffer + pb, len); 
  }   
else
  {
  strncpy(buffer, cmd_buffer + pb, len);
  buffer[len] = 0;
  }  

len = strlen(buffer);

/* There must be at least one '/' in the string, because of the checking
done above. */

s = buffer + len - 1;
while (s > buffer && *s != '/') s--;

*s = 0;
endlen = len - (s - buffer) - 1;
dir = opendir(buffer);
if (dir == NULL) goto RETURN;

while ((dent = readdir(dir)) != NULL)
  {
  if (strcmp(dent->d_name, ".") == 0 || strcmp(dent->d_name, "..") == 0)
    continue; 
  if (strncmp(dent->d_name, s + 1, endlen) == 0)
    {
    if (!insert_found)
      { 
      strcpy(insert, dent->d_name + endlen);  
      insert_found = TRUE; 
      beep = FALSE; 
      }
    else
      {
      int i;
      beep = TRUE; 
      for (i = 0; i < strlen(insert); i++)
        {
        if (insert[i] != dent->d_name[endlen + i]) break;
        }   
      insert[i] = 0;
      }      
    }  
  }
closedir(dir);  

if (insert_found && insert[0] != 0)
  {
  int inslen = strlen(insert);
  memmove(cmd_buffer + p + inslen, cmd_buffer + p, strlen(cmd_buffer + p));
  memcpy(cmd_buffer + p, insert, inslen);
  *pmaxptr = pmax + inslen;  
  p += inslen;
  } 

RETURN:
if (beep) sys_beep();
return p;
}


/*************************************************
*             Generate crash file name           *
*************************************************/

char *sys_crashfilename(int which)
{
return which? "NEcrash" : "NEcrashlog";
}


/*************************************************
*              Open a file                       *
*************************************************/

/* Interpretation of file names beginning with ~ is done here.
As Unix is not a file-typed system, the filetype argument is
ignored. */

FILE *sys_fopen(char *name, char *type, int *filetype)
{
char buff[256];
if (name[0] == '~') name = sort_twiddle(name, strlen(name), buff);

/* Handle optional automatic backup for output files. We add "~"
to the name, as is common on Unix. */

if (main_backupfiles && strcmp(type, "w") == 0 && !file_written(name))
  {
  char bakname[80];
  strcpy(bakname, name);
  strcat(bakname, "~");
  remove(bakname);
  rename(name, bakname);
  file_setwritten(name);
  }

return fopen(name, type);
}


/*************************************************
*              Check file name                   *
*************************************************/

char *sys_checkfilename(char *s, BOOL reading)
{
unsigned char *p = (unsigned char *)s;

while (*p != 0)
  {
  if (*p == ' ')
    {
    while (*(++p) != 0)
      {
      if (*p != ' ') return "(contains a space)";
      }
    break;
    }
  if (*p < ' ' || *p > '~') return "(contains a non-printing character)";
  p++;
  }

return NULL;
}


/*************************************************
*            Set a file's type                   *
*************************************************/

/* Unix does not use file types. */

void sys_setfiletype(char *name, int type)
{
}



/*************************************************
*         Give number of characters for newline  *
*************************************************/

int sys_crlfcount(void)
{
return 1;
}



/*************************************************
*            Write to message stream             *
*************************************************/

/* Output to msgs_fid is passed through here, so that it can
be handled system-specifically if necessary. */

void sys_mprintf(FILE *f, char *format, ...)
{
va_list ap;
va_start(ap, format);
vfprintf(f, format, ap);
va_end(ap);
}



/*************************************************
*            Give reason for key refusal         *
*************************************************/

/* This procedure gives reasons for the setting of bits
in the maps that forbid certain keys being set. */

char *sys_keyreason(int key)
{
switch (key & ~(s_f_shiftbit+s_f_ctrlbit))
  {
  case s_f_bsp: return "backspace is equivalent to ctrl/h";
  case s_f_ret: return "return is equivalent to ctrl/m";
  default:      return "not available on keyboard";
  }
}



/*************************************************
*           System-specific help info            *
*************************************************/

BOOL sys_help(char *s)
{
s = s;
return FALSE;
}



/*************************************************
*           System-specific interrupt check      *
*************************************************/

/* This is called from main_interrupted(). This in turn is called only
when the main part of NE is in control, not during screen editing operations.
We have to check for ^C by steam, as we are running in raw terminal mode.

It turns out that the ioctl can be quite slow, so we don't actually want
to do it for every line in, e.g., an m command. NE now maintains a count of
calls to main_interrupted(), reset when a command line is read, and it also
passes over the kind of processing that is taking place:

  ci_move     m, f, bf, or g command
  ci_type     t or tl command
  ci_read     reading command line or data for i command
  ci_cmd      about to obey a command
  ci_delete   deleting all lines of a buffer
  ci_scan     scanning lines (e.g. for show wordcount)
  ci_loop     about to obey the body of a loop

These are successive integer values, starting from zero. */


/* This vector of masks is used to mask the counter; if the result is
zero, the ioctl is done. Thereby we have different intervals for different
types of activity with very little cost. The numbers are pulled out of the 
air... */

static int ci_masks[] = {
    1023,    /* ci_move - every 1024 lines*/
    0,       /* ci_type - every time */
    0,       /* ci_read - every time */
    15,      /* ci_cmd  - every 16 commands */
    127,     /* ci_delete - every 128 lines */
    1023,    /* ci_scan - every 1024 lines */
    15       /* ci_loop - every 16 commands */
};

void sys_checkinterrupt(int type)
{
if (main_screenOK && !main_escape_pressed &&
    (main_cicount & ci_masks[type]) == 0)
  {
  int c = 0;
  ioctl(ioctl_fd, FIONREAD, &c);
  while (c-- > 0)
    {
    if (getchar() == tc_int_ch) main_escape_pressed = TRUE;
    }
  }
}



/*************************************************
*           Tidy up after interrupted fgets      *
*************************************************/

/* No tidying needed for Unix */

void sys_after_interrupted_fgets(BOOL buffergiven, char *buffer)
{
}



/*************************************************
*       Display hourglass during commands        *
*************************************************/

/* This function is called while obeying command lines from
an interactive session. It can be used to display an hourglass
or any other indication of busy-ness. */

void sys_hourglass(BOOL on)
{
}


/* End of sysunix.c */
