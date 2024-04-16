/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1993, 1994 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: October 1994 */


/* This file contains screen-handling code for use with termcap
functions under Unix. Now extended to use terminfo as an alter-
native, depending on a configuration option. */


#include "ehdr.h"
#include "keyhdr.h"
#include "shdr.h"
#include "scomhdr.h"
#include "unixhdr.h"

#define sh  s_f_shiftbit
#define ct  s_f_ctrlbit
#define shc (s_f_shiftbit + s_f_ctrlbit)


#ifndef HAVE_TERMCAP
#define tgoto(a, b, c) tparm(a, c, b)
#endif





/*************************************************
*            Main terminal control data          *
*************************************************/

int tc_n_co;        /* columns on screen */
int tc_n_li;        /* lines on screen */

char *tc_s_al;      /* add new line */
char *tc_s_bc;      /* cursor left - used only if NoZero */
char *tc_s_ce;      /* clear to end of line */
char *tc_s_cl;      /* clear screen */
char *tc_s_cm;      /* cursor move */
char *tc_s_cs;      /* set scrolling region */
char *tc_s_dc;      /* delete char */
char *tc_s_dl;      /* delete line */
char *tc_s_ic;      /* insert character */
char *tc_s_ip;      /* insert padding */
char *tc_s_ke;      /* reset terminal */
char *tc_s_ks;      /* set up terminal */
char *tc_s_pc;      /* pad character */
char *tc_s_se;      /* end standout */
char *tc_s_sf;      /* scroll text up */
char *tc_s_so;      /* start standout */
char *tc_s_sr;      /* scroll text down */
char *tc_s_te;      /* end screen management */
char *tc_s_ti;      /* start screen management */
char *tc_s_up;      /* cursor up - user only if NoZero */

int tc_f_am;        /* automatic margin flag */

unsigned char *tc_k_trigger;  /* trigger char table for special keys */
unsigned char *tc_k_strings;  /* strings for keys 0-n and specials */

int tt_special;     /* terminal special types */
int tc_int_ch;      /* interrupt char */
int tc_stop_ch;     /* stop char */
int NoZero;         /* never generate zero row or col */
int ioctl_fd;       /* FD to use for ioctl calls */
int window_changed; /* SIGWINSZ received */



/*************************************************
*             Static Data                        *
*************************************************/

static int sunix_setrendition = s_r_normal;

/* It turns out to be much faster to buffer up characters for
output, at least under some systems and some libraries. SunOS 4.1.3
with the GNU compiler is a case in point. */

#define outbuffsize 4096

static char out_buffer[outbuffsize];
static int outbuffptr = 0;

static struct termios oldtermparm;

static unsigned char Pkeytable[] = {
  s_f_del,                /* Table starts at char 127 */
  s_f_cup,
  s_f_cdn,
  s_f_clf,
  s_f_crt,
  s_f_cup+s_f_shiftbit,
  s_f_cdn+s_f_shiftbit,
  s_f_clf+s_f_shiftbit,
  s_f_crt+s_f_shiftbit,
  s_f_cup+s_f_ctrlbit,
  s_f_cdn+s_f_ctrlbit,
  s_f_clf+s_f_ctrlbit,
  s_f_crt+s_f_ctrlbit,
  s_f_reshow,
  s_f_del,
  s_f_del+s_f_shiftbit,
  s_f_del+s_f_ctrlbit,
  s_f_bsp,
  s_f_bsp+s_f_shiftbit,
  s_f_bsp+s_f_ctrlbit,     
  s_f_copykey,
  s_f_copykey+s_f_shiftbit,
  s_f_copykey+s_f_ctrlbit,  
  s_f_hom,
  s_f_tab+s_f_ctrlbit,    /* use ctrl+tab for back tab */
  s_f_tab+s_f_ctrlbit,    /* true ctrl+tab */ 
  s_f_ins,
  s_f_user,               /* next=data shouldn't get used */
  s_f_user };




/*************************************************
*           Flush buffered output                *
*************************************************/

/* Make sure all output has been delivered */

static void sunix_flush(void)
{
if (outbuffptr > 0)
  {
  write(ioctl_fd, out_buffer, outbuffptr);
  outbuffptr = 0;
  }
}


/*************************************************
*              Accept keystroke                  *
*************************************************/

void sys_keystroke(int key)
{
if (setjmp(error_jmpbuf) == 0)
  {
  error_longjmpOK = TRUE;
  if (key < 32) key_handle_function(key);
    else if (key >= 127)
      {
      if (key >= Pkey_f0) key_handle_function(s_f_umax + key - Pkey_f0);
        else key_handle_function(Pkeytable[key - 127]);
      }
    else key_handle_data(key);
  }
error_longjmpOK = FALSE;
}




/*************************************************
*      Get keystroke and convert to standard     *
*************************************************/

/* This function has to take into account function
key sequences and yield the next logical keystroke.
If it reads a number of characters and then discovers
that they do not form an escape sequence, it returns
the first of them, leaving the remainder on a stack
which is "read" before next doing a real read. 

We should never get EOF from a raw mode terminal input
stream, but its possible to get NE confused into thinking
it is interactive when it isn't. So code for this case. */

static char kbback[20];
static int kbbackptr;

static int sunix_nextchar(int *type)
{
int scount, kbptr, c, k;
unsigned char *sp;
unsigned char kbbuff[20];

sunix_flush();  /* Deliver buffered output */

/* Get next key */

if (kbbackptr > 0) c = kbback[--kbbackptr]; else
  {
  c = getchar();
  if (c == EOF) return -1;
  }   

k = tc_k_trigger[c];

*type = -1; /* unset - set only for forced data */

/* An entry of 254 in the table indicates the start of a
multi-character escape sequence; 255 => nothing special;
other values => single char control sequence. */

if (k != 254) return (k == 255)? c : k;

/* We are at the start of a possible multi-character sequence. */

sp = tc_k_strings + 1;
scount = tc_k_strings[0];
kbptr = 0;

/* Loop reading characters and checking strings against
those in the list. */

for (;;)
  {
  if (c == sp[1+kbptr])
    {
    if (sp[2+kbptr] == 0)
      {
      c = sp[3+kbptr];
      if (c == Pkey_data)
        {
        c = (kbbackptr > 0)? kbback[--kbbackptr] : getchar();
        if (c < 127) c &= ~0x60; 
        *type = ktype_data;
        }
      break;
      }
    kbbuff[kbptr++] = c;
    c = (kbbackptr > 0)? kbback[--kbbackptr] : getchar();
    continue;
    }

  /* Failed to match current escape sequence. Stack characters
  read so far and either advance to next sequence, or yield one
  character. */

  kbbuff[kbptr++] = c;
  while (kbptr > 1) kbback[kbbackptr++] = kbbuff[--kbptr];
  c = kbbuff[--kbptr];
  if (--scount > 0) sp += sp[0]; else break;
  }

return c;
}


/*************************************************
*       Get keystroke for command line           *
*************************************************/

/* This function is called when reading a command line in screen
mode. It must return a type and a value. We must notice the user's
interrupt key by hand - luckily it is usually ^C and so not a key
used to edit the line in the default NE binding. */

int sys_cmdkeystroke(int *type)
{
int key = sunix_nextchar(type);

if (key == oldtermparm.c_cc[VINTR]) main_escape_pressed = TRUE;

if (*type < 0)
  {
  if (key < 32) *type = ktype_ctrl;
    else if (key >= 127)
      {
      *type = ktype_function;
      key = (key >= Pkey_f0)? s_f_umax + key - Pkey_f0 : Pkeytable[key - 127];
      }
    else *type = ktype_data;
  }
return key;
}




/*************************************************
*            Output termcap/info string          *
*************************************************/

/* General buffered output routine */

#ifndef MY_PUTC_ARG_TYPE                
#define MY_PUTC_ARG_TYPE int
#endif
                    
static int my_putc(MY_PUTC_ARG_TYPE c)
{
if (outbuffptr > outbuffsize - 2)
  {
  write(ioctl_fd, out_buffer, outbuffptr);
  outbuffptr = 0;
  }
out_buffer[outbuffptr++] = c;
return c;
}


#ifdef HAVE_TERMCAP
static void outTCstring(char *s, int amount)
{
int pad = 0;
while ('0' <= *s && *s <= '9') pad = pad*10 + *s++ - '0';
pad *= 10;
if (*s == '.' && '0' <= *(++s) && *s <= '9') pad += *s++ - '0';
if (*s == '*') { s++; pad *= amount; }
/* while (*s) fputc(*s++, stdout); */
while (*s) my_putc(*s++);
if (pad)
  {
  int pc = (tc_s_pc == NULL)? 0 : *tc_s_pc;
  if ((pad % 10) >= 5) pad += 10;
  pad /= 10;
  while (pad--) my_putc(pc);
  }
}

#else
#define outTCstring(s, amount) tputs(s, amount, my_putc)
#endif



/*************************************************
*         Interface routines to scommon          *
*************************************************/

static void sunix_move(int x, int y)
{
if (!NoZero || (x > 0 && y > 0))
  {
  outTCstring(tgoto(tc_s_cm, x, y), 0);
  }
else
  {
  int left = (x == 0)? 1 : 0;
  int up =   (y == 0)? 1 : 0;
  outTCstring(tgoto(tc_s_cm, x+left, y+up), 0);
  if (up) outTCstring(tc_s_up, 0);
  if (left)
    {
    if (tc_s_bc == NULL) my_putc(8); else outTCstring(tc_s_bc, 0);
    }
  }
}


static void sunix_rendition(int r)
{
if (r != sunix_setrendition)
  {
  sunix_setrendition = r;
  outTCstring((r == s_r_normal)? tc_s_se : tc_s_so, 0);
  }
}


static void sunix_putc(int c)
{
my_putc((ch_tab2[c] || (main_eightbit && (c & 255) >= topbit_minimum))? 
  c : '?');
}


static void sunix_cls(int bottom, int left, int top, int right)
{
if (bottom != screen_max_row || left != 0 || top != 0 ||
    right != screen_max_col || tc_s_cl == NULL)
  {
  int i;
  for (i = top; i <= bottom;  i++)
    {
    sunix_move(left, i);
    if (tc_s_ce != NULL) outTCstring(tc_s_ce, 0); else
      {
      int j;
      sunix_rendition(s_r_normal);
      for (j = left; j <= right; j++) sunix_putc(' ');
      }
    }
  }
else outTCstring(tc_s_cl, 0);
}


/* Either "set scrolling region" and "scroll reverse" will be available,
or "delete line" and "insert line" will be available. (It is possible
that "set scrolling region", "delete line" and "insert line" will be
available without "scroll reverse". We can handle this too.) 

Experience shows that some terminals don't do what you expect if the
region is set to one line only; if these have delete/insert line, we
can use that instead (the known cases do). */

static void sunix_vscroll(int bottom, int top, int amount, BOOL deleting)
{
int i;
if (amount > 0)
  {
  if (tc_s_cs != NULL && tc_s_sr != NULL)
    {
    outTCstring(tgoto(tc_s_cs, bottom, top), 0);
    sunix_move(0, top);
    for (i = 0; i < amount; i++) outTCstring(tc_s_sr, 0);
    outTCstring(tgoto(tc_s_cs, screen_max_row, 0), 0);
    }
  else for (i = 0; i < amount; i++)
    {
    sunix_move(0, bottom);
    outTCstring(tc_s_dl, screen_max_row - bottom);
    sunix_move(0, top);
    outTCstring(tc_s_al, screen_max_row - top);
    }
  }

else
  {
  amount = -amount;
  if (tc_s_cs != NULL && (top != bottom || tc_s_dl == NULL))
    {
    outTCstring(tgoto(tc_s_cs, bottom, top), 0);
    sunix_move(0, bottom);
    for (i = 0; i < amount; i++)
      if (tc_s_sf == NULL) my_putc('\n');
        else outTCstring(tc_s_sf, 0);
    outTCstring(tgoto(tc_s_cs, screen_max_row, 0), 0);
    }

  else for (i = 0; i < amount; i++)
    {
    sunix_move(0, top);
    outTCstring(tc_s_dl, screen_max_row - top);
    if (bottom != screen_max_row)
      {
      sunix_move(0, bottom);
      outTCstring(tc_s_al, screen_max_row - bottom);
      }
    }
  }
}




/*************************************************
*                 Set terminal state             *
*************************************************/

/* The set state is raw mode (cbreak mode having been found
impossible to work with.) Once set up, output the ks string
if it exists. */

static void setupterminal(void)
{
struct termios newparm;
newparm = oldtermparm;
newparm.c_iflag &= ~(IGNCR | ICRNL);
newparm.c_lflag &= ~(ICANON | ECHO | ISIG);
newparm.c_cc[VMIN] = 1;
newparm.c_cc[VTIME] = 0;
newparm.c_cc[VSTART] = 0;
newparm.c_cc[VSTOP] = 0;
#ifndef NO_VDISCARD
newparm.c_cc[VDISCARD] = 0;
#endif

tcsetattr(ioctl_fd, TCSANOW, &newparm);

/* If this is a ttpa terminal, we can inquire about the screen size */

if (tt_special == tt_special_ttpa)
  {
  int ch;
  my_putc(escch);       /* ESC Z is an enquiry */
  my_putc('Z');
  sunix_flush();  
 
  while ((ch = getchar()) != escch && ch != EOF);
  if (ch == escch)
    {
    ch = getchar();   
    if (ch == 'Y')
      {
      ch = getchar();                                   /* Note careful */
      tc_n_li = (ch - 31) + ((getchar() - 32) << 6);    /* coding here. */
      ch = getchar(); 
      tc_n_co = (ch - 31) + ((getchar() - 32) << 6);
      }
    } 
  }

/* Otherwise, get the screen size again in case it's changed */

else
  {
#ifdef unixwinsz
  struct winsize parm;
  if (ioctl(ioctl_fd, TIOCGWINSZ, &parm) == 0)
    {
    if (parm.ws_row != 0) tc_n_li = parm.ws_row;
    if (parm.ws_col != 0) tc_n_co = parm.ws_col;
    }
#endif
  }

/* Set max rows/cols */

screen_max_row = tc_n_li -1; 
screen_max_col = tc_n_co -1; 

/* Start screen management and enable keypad if required */

if (tc_s_ti != NULL) outTCstring(tc_s_ti, 0);   /* start screen management */
if (tc_s_ks != NULL) outTCstring(tc_s_ks, 0);   /* enable keypad */
}


/*************************************************
*                  Unset terminal state          *
*************************************************/

static void resetterminal(void)
{
if (tc_s_ke != NULL) outTCstring(tc_s_ke, 0);
if (tc_s_te != NULL) outTCstring(tc_s_te, 0);
sunix_flush();
tcsetattr(ioctl_fd, TCSANOW, &oldtermparm);
}



/*************************************************
*         Position to screen bottom for crash    *
*************************************************/

/* This function is called when about to issue a disaster error message.
It should position the cursor to a clear point at the screen bottom, and
reset the terminal if necessary. */

void sys_crashposition(void)
{
sunix_rendition(s_r_normal);
sunix_move(0, screen_max_row);
resetterminal();
main_screenmode = FALSE;
}




/*************************************************
*                Initialize                      *
*************************************************/

static void tc_init(void)
{
/* Set up maps for which keys may be used. */

key_controlmap    = 0xFFFFFFFE;    /* exclude nothing */
key_functionmap   = 0x3FFFFFFE;    /* allow 1-29 */
key_specialmap[0] = 0x0000001F;    /* del, arrows */
key_specialmap[1] = 0x00000000;    /* nothing shifted */
key_specialmap[2] = 0x00000000;    /* nothing ctrled */
key_specialmap[3] = 0x00000000;    /* nothing shifted+ctrled */

/* Additional keys on some special terminals */

if (tt_special == tt_special_ArcTermulator)
  {
  key_specialmap[0] = 0x0000231F;   /* copy, home, insert, del, arrows */
  key_specialmap[1] = 0x00000003;   /* shifted arrows, up/down only */
  key_specialmap[2] = 0x00000000;   /* nothing */
  }
else if (tt_special == tt_special_ttpa)
  {
  key_specialmap[0] = 0x0000201F;   /* copy, del, arrows */
  key_specialmap[1] = 0x0000000F;   /* shifted arrows */
  key_specialmap[2] = 0x0000000F;   /* ctrled arrows */
  }
else if (tt_special == tt_special_xterm);
  {
  key_specialmap[1] = 0x0000008F;   /* shifted tab, shifted arrows */
  key_specialmap[2] = 0x0000008F;   /* ctrl tab, ctrl arrows */
  }



#ifdef undef
/* Miscellaneous initialization */

splitenabled = TRUE;
delenabled = TRUE;
osenabled = FALSE;
CmdReading = -1;
StopOnEscape = FALSE;
suspendflag = FALSE;

rendition = s_r.normal;
setrendition = rendition;

/* Default tab stops every eight places */

tabs = getvec(maxtabs+1);
tabs[0] = maxtabs;
for (i = 1; i < maxtabs; i++) tabs[i] = i*8;
#endif

/* Read and save original terminal state; set up terminal */

tcgetattr(ioctl_fd, &oldtermparm);
tc_int_ch = oldtermparm.c_cc[VINTR];
tc_stop_ch = oldtermparm.c_cc[VSTOP];

setupterminal();
}




/*************************************************
*       Entry point for non-window screen run    *
*************************************************/



void sys_runscreen(void)
{
FILE *fid = NULL;
char *fromname = arg_from_name;
char *toname = (arg_to_name == NULL)? arg_from_name : arg_to_name;

sys_w_cls = sunix_cls;
sys_w_flush = sunix_flush;
sys_w_move = sunix_move;
sys_w_rendition = sunix_rendition;
sys_w_putc = sunix_putc;

sys_setupterminal = setupterminal;
sys_resetterminal = resetterminal;

sys_w_hscroll = NULL;
sys_w_vscroll = sunix_vscroll;

arg_from_name = arg_to_name = NULL;
currentbuffer = main_bufferchain = NULL;

/* If there's an input file, check that it exists before
initializing the screen. */

if (fromname != NULL && strcmp(fromname, "-") != 0)
  {
  fid = sys_fopen(fromname, "r", NULL);
  if (fid == NULL)
    {
    printf("** The file \"%s\" could not be opened.\n", fromname);
    printf("** NE abandoned.\n");
    main_rc = 16;   
    return;
    }
  }
  
/* Now set up the screen */       

tc_init();
s_init(screen_max_row, screen_max_col, TRUE);
scrn_init(TRUE);
scrn_windows();                     /* cause windows to be defined */
main_screenOK = TRUE;
default_rmargin = main_rmargin;     /* first one = default */
 
if (init_init(fid, fromname, 0, toname, main_stream))
  {
  int x, y;

  if (!main_noinit && main_einit != NULL) cmd_obey(main_einit);
  main_initialized = TRUE;
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
    main_shownlogo = TRUE;
    if (key_table[7] == ka_rc && screen_max_col > 64) /* The default config */
      s_printf("  To exit, type ^G, W, Return.");  
    s_selwindow(first_window, x, y);
    } 

  kbbackptr = 0;

  while (!main_done)
    {
    int type;
    int c = sunix_nextchar(&type);

    main_rc = error_count = 0;           /* reset at each interaction */

    /* If the window changes we get back a zero value, but we don't
    want to do anything with it. */
      
    #ifdef unixwinsz
    if (window_changed)
      {
      struct winsize parm;
      if (ioctl(ioctl_fd, TIOCGWINSZ, &parm) == 0)
        {
        if (parm.ws_row != 0) tc_n_li = parm.ws_row;
        if (parm.ws_col != 0) tc_n_co = parm.ws_col;
        screen_max_row = tc_n_li - 1;
        screen_max_col = tc_n_co - 1;
        scrn_setsize(); 
        }
      window_changed = FALSE;
      }  
    else   
    #endif
      {
      if (c == -1) main_done = TRUE;    /* Should never occur, but... */
        else if (type < 0) sys_keystroke(c); else key_handle_data(c);
      } 
    }
  }

sunix_rendition(s_r_normal);
resetterminal();
close(ioctl_fd);
}


/* End of sunix.c */

