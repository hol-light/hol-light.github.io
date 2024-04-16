/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991-2002           *
 *                                                            *
 * Written by Philip Hazel, starting November 1991            *
 * Adapted for the PC by Brian Holley, starting November 1993 *
 * This file last modified: Nov 2002                          *
 *  added WIN32 Console support                               *
 *                                                            *
 * This PC version is designed for Borland C++ (C code only)  *
 * versions 3.1 or later; WIN32 Console code also compatible  *
 * with Microsoft Visual C v. 5.0                             *
 *                                                            *
 * This file contains the system-specific routines for the PC */

/* Implementation notes                                                  *
 * Altc, Altq, Alts can be used as alternatives to ctr-c, ctrl-q, ctrl-s *
 * argument added:                                                       *
 *   -necol textcolour,backcolour,invtextcol,invbackcol                  *
 *   where the four colours can each be one of:                          *
 *   black,blue,green,cyan,red,magenta,brown,white,grey,pink,yellow      *
 *   each can be prefixed with 'light_' (e.g. 'light_blue')              *
 *   (default colours are derived from the console or shortcut settings) */

#include "elocal.h"
#ifdef __WIN32CON__
#include <windows.h>
#endif
#include "ehdr.h"
#include "shdr.h"
#include "scomhdr.h"
#include "keyhdr.h"

#ifdef __WIN32CON__
  HANDLE OutConsoleHandle, InConsoleHandle;
  struct _CONSOLE_SCREEN_BUFFER_INFO OutConsole,InConsole;
  struct _COORD cursorpos;
  UINT codepage;
#else

#include <dos.h>
#include <bios.h>
#include <dir.h>
extern unsigned _stklen = 16384;
extern int directvideo = 1 ;
extern int _wscroll;
#include <conio.h>

#endif

/* Function declarations */

static int nextchar(int *type);
static int getachar(void);
static void pc_cls(int bottom, int left, int top, int right);
static void pc_move(int x, int y);
static void pc_rendition(int r);
static void pc_putc(int c);
/*static void pc_hscroll(int left, int bottom, int right, int top, int amount);*/
static void pc_vscroll(int bottom, int top, int amount, BOOL deleting);
static void setupterminal(void);
static void resetterminal(void);
static void pc_flush(void);
#ifdef __WIN32CON__
int __cdecl getch(void);
int __cdecl kbhit(void);
#ifndef Debug
  #ifndef TraceStore
   #define this_version "(PC-Win32 1.2)"
  #else
   #define this_version "(Tracing)"
  #endif
#else
  #define this_version "(Debugging)"
#endif
#else
#ifndef Debug
  #ifndef TraceStore
   #define this_version "(PC-DOS 1.2)"
  #else
   #define this_version ")(Tracing)"
  #endif
#else
  #define this_version ")(Debugging)"
#endif
#endif

#define sh  s_f_shiftbit
#define ct  s_f_ctrlbit
#define shc (s_f_shiftbit + s_f_ctrlbit)

#ifdef __WIN32CON__
struct text_info {
  unsigned short winleft;
  unsigned short wintop;
  unsigned short winright;
  unsigned short winbottom;
  unsigned short attribute;
  unsigned short normattr;
  unsigned short currmode;
  unsigned short screenheight;
  unsigned short screenwidth;
  unsigned short curx;
  unsigned short cury;
};
static startrow, lastrow,lastcol;
#endif

static struct text_info scrn_info;
static char ESCAPED;
extern char *arg_zero;
int colour_set;
unsigned caller_colour;
FILE *helpfile;

/* Control keystrokes to return for keys that are not
dealt with locally nor are ctrls. */

/* Offsets into the table are keyboard scan codes minus 14 */

static short int ControlKeystroke[] = {
                                      s_f_bsp,     /* 7F     */
  s_f_tab,                                         /* 0x0f.. backtab */
  17      ,   s_f_user,   s_f_user,   s_f_user,    /* 0x10.. altq      altw     alte     altr */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x14.. altt      alty     altu     alti */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x18.. alto      altp     alt[     alt] */
  s_f_ret,    s_f_user,   s_f_user,   19      ,    /* 0x1c.. enter     ctrl     alta     alts */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x20.. altd      altf     altg     alth */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x24.. altj      altk     altl     alt; */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x28.. alt'      alt`     lshift   alt\ */
  s_f_user,   s_f_umax+3, 3,          s_f_user,    /* 0x2c.. altz      altx     altc     altv */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x30.. altb      altn     altm     alt, */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x34.. alt.      alt/     rshift   altgrey* */
  s_f_user,   s_f_user,   s_f_user,   s_f_umax+1,  /* 0x38.. alt       altspace capslk   f1 */
  s_f_umax+2, s_f_umax+3, s_f_umax+4, s_f_umax+5,  /* 0x3c.. f2        f3       f4       f5 */
  s_f_umax+6, s_f_umax+7, s_f_umax+8, s_f_umax+9,  /* 0x40.. f6        f7       f8       f9 */
  s_f_umax+10,s_f_user,   s_f_user,   s_f_hom,     /* 0x44.. f10       numlk    scrlk    home */
  s_f_cup,    s_f_pup,    s_f_user,   s_f_clf,     /* 0x48.. up        pgup     grey-    left */
  s_f_user,   s_f_crt,    s_f_user,   s_f_end,     /* 0x4c.. kp5       right    grey+    endkey */
  s_f_cdn,    s_f_pdn,    s_f_ins,    s_f_del,     /* 0x50.. down      pgdn     inskey   del */
  s_f_umax+11,s_f_umax+12,s_f_umax+13,s_f_umax+14, /* 0x54.. shiftf1   shiftf2  shiftf3  shiftf4 */
  s_f_umax+15,s_f_umax+16,s_f_umax+17,s_f_umax+18, /* 0x58.. shiftf5   shiftf6  shiftf7  shiftf8 */
  s_f_umax+19,s_f_umax+20,s_f_umax+21,s_f_umax+22, /* 0x5c.. shiftf9   shiftf10 ctrlf1   ctrlf2 */
  s_f_umax+23,s_f_umax+24,s_f_umax+25,s_f_umax+26, /* 0x60.. ctrlf3    ctrlf4   ctrlf5   ctrlf6 */
  s_f_umax+27,s_f_umax+28,s_f_umax+29,s_f_umax+30, /* 0x64.. ctrlf7    ctrlf8   ctrlf9   ctrlf10 */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x68.. altf1     altf2    altf3    altf4 */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x6c.. altf5     altf6    altf7    altf8 */
  s_f_user,   s_f_user,   s_f_user,   s_f_wordlf,  /* 0x70.. altf9     altf10   ?        ctrlleft */
  s_f_wordrt, s_f_umax+8, s_f_pdn+ct, s_f_umax+18, /* 0x74.. ctrlright ctrlend  ctrlpgdn ctrlhome */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x78.. alt1      alt2     alt3     alt4 */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x7c.. alt5      alt6     alt7     alt8 */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x80.. alt9      alt0     alt-     alt= */
  s_f_pup+ct, s_f_user,   s_f_user,   s_f_user,    /* 0x84.. ctrlpgup  f11      f12      shiftf11 */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x88.. shiftf12  ctrlf11  ctrlf12  altf11 */
  s_f_user,   s_f_cup+ct, s_f_user,   s_f_user,    /* 0x8c.. altf12    ctrlup   ?        ?    */
  s_f_user,   s_f_cdn+ct, s_f_ins+ct, s_f_del+ct,  /* 0x90.. ?         ctrldown ?        ctrldel */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x94.. */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x98.. */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0x9c.. */
  s_f_user,   s_f_user,   s_f_user,   s_f_user,    /* 0xa0.. */
  s_f_user,   s_f_user,   s_f_user,                /* 0xa4..0xa6 */

/* 0xa7 onwards are pseudo key codes - shifted edit keys */
            s_f_hom+sh,  /*   0xa7 */
  s_f_cup+sh, s_f_pup+sh, s_f_user+sh,s_f_clf+sh,       /* 0xa8.. */
  s_f_user+sh,s_f_crt+sh, s_f_user+sh,s_f_end+sh,       /* 0xac.. */
  s_f_cdn+sh, s_f_pdn+sh, s_f_ins+sh, s_f_del+sh,       /* 0xb0.. */
  s_f_bsp+sh, s_f_tab+sh, s_f_bsp+ct, s_f_tab+ct,      /* 0xb4.. */
  s_f_ret+sh, s_f_ret+ct};                              /* 0xb8.. */

/* List of signals to be trapped for buffer dumping on
crashes to be effective. Names are for use in messages. */

int signal_list[] = {
  SIGABRT, SIGILL, SIGFPE, SIGSEGV, SIGTERM, -1 };

char *signal_names[] = {
  "SIGABRT", "SIGILL", "SIGFPE", "SIGSEGV", "SIGTERM", "" };


/*************************************************
*               Static data                      *
*************************************************/

/* The following keystrokes are not available and so their default
settings must be squashed. Note that we leave "ctrl/tab", because
esc tab maps to it and is suitable translated for output. For some
known terminals (e.g. Archimedes ttp) tsome of hese keys are not
squashed, because they are built into this code. There are special
versions of the table for these. */
/*
static unsigned char non_keys[] = {
  s_f_copykey, s_f_copykey+sh, s_f_copykey+ct,
  s_f_cup+sh, s_f_cup+sh+ct, s_f_cdn+sh,  s_f_cdn+sh+ct,
  s_f_clf+sh, s_f_clf+sh+ct, s_f_crt+sh,  s_f_crt+sh+ct,
  s_f_del+sh, s_f_del+sh+ct, s_f_bsp+sh,  s_f_bsp+sh+ct,
  s_f_ret+sh, s_f_ret+sh+ct, s_f_tab+sh,  s_f_tab+sh+ct,
  s_f_ins+sh, s_f_ins+sh+ct, s_f_hom+sh,  s_f_hom+sh+ct,
  s_f_pup+sh, s_f_pup+sh+ct, s_f_pdn+sh,  s_f_pdn+sh+ct,
  s_f_end+sh, s_f_end+sh+ct,
  s_f_tab+ct, s_f_ins+ct,  s_f_del+ct,    s_f_bsp+ct,
  0 };
*/
static unsigned char non_keys[] = {
  s_f_copykey, s_f_copykey+sh, s_f_copykey+ct,
  s_f_cup+sh+ct, s_f_cdn+sh+ct,
  s_f_clf+sh+ct, s_f_crt+sh+ct,
  s_f_del+sh+ct, s_f_bsp+sh+ct,
  s_f_tab+sh+ct,
  s_f_ins+sh+ct, s_f_hom+sh+ct,
  s_f_pup+sh+ct, s_f_pdn+sh+ct,
  s_f_end+sh+ct,
  0 };
/*************************************************
*          Add to arg string if needed           *
*************************************************/

/* This function permits the system-specific code to add additional
argument definitions to the arg string which will be used to decode
the command line. */

char *sys_argstring(char *s)
{ strcat(s, ",necol/k");
  return s;
}

static void setupcolours(char *ne_colour)
{ char *next_colour,*colour,fgbg[4],colr,i=0,colourcpy[64];
  char colours[]=" black   blue    green   cyan    red     magenta brown   white   grey    pink    yellow  ";
  fgbg[0] = fgbg[1] = fgbg[2] = fgbg[3] = 999;
  if (ne_colour)
  {
#ifdef __WIN32CON__
    scrn_info.normattr=OutConsole.wAttributes;
#else
    gettextinfo(&scrn_info);
#endif
    scrn_info.attribute =(short)( ((scrn_info.normattr%16*16))+scrn_info.normattr/16);
    if (!colour_set) caller_colour=scrn_info.normattr;
    if (strlen(ne_colour)<64)
    { strcpy(colourcpy,ne_colour);
      next_colour = strtok(colourcpy, ", ");
      do
      { colr = 127;
        if (!strncmp(next_colour,"light_",6))
        { colour = strstr(colours,next_colour+6);
//          if (colour && (i % 2) == 0)
            colr =(char)( ((int)(colour-colours))/8 + 8);
        }
        else
        { colour = strstr(colours,next_colour);
          if (colour)
          { colr = (char)((colour-colours)/8);
            if (colr > 8) colr =(char)(colr + 4);
          }
        }
        next_colour = strtok(NULL, ", ");
        fgbg[i++] = colr;
      } while (next_colour && i<4);
      if (fgbg[2]!=999 && fgbg[3]!=999) scrn_info.attribute =(short)((fgbg[3]<<4)+fgbg[2]);
      if (fgbg[0]!=999 && fgbg[1]!=999)
      { scrn_info.normattr = (short)((fgbg[1]<<4)+fgbg[0]);
        pc_rendition(s_r_normal);
        colour_set=TRUE;
      }
    }
    else
    {;
    }
  }
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
{ int destination, source;
#ifdef __WIN32CON__
  struct _WIN32_FIND_DATAA fileinfo;
  HANDLE findhandle;
#else
  struct ffblk fileinfo;
#endif
  char *ptr, *new_arg_buff;
  if (n==27) setupcolours(result[n].data.text);
  if (result[0].data.text != NULL)
  { for (destination = 1; destination < 9; destination++)
    if (result[destination].data.text == NULL) break;
    if (destination<9)
    { for (source=0;source<destination;source++)
      { if (strchr(result[source].data.text,'*')
          || strchr(result[source].data.text,'?'))
        { ptr=result[source].data.text;
#ifdef __WIN32CON__
        if ((findhandle=FindFirstFile(ptr,&fileinfo))!=INVALID_HANDLE_VALUE)
#else
        if (findfirst(ptr,&fileinfo,0)==NULL)
#endif
          { new_arg_buff=store_Xget(strlen(ptr)+16L);
            strcpy(new_arg_buff,ptr);
            ptr=strrchr(new_arg_buff,'\\');
            if (ptr==NULL) ptr=strrchr(new_arg_buff,':');
            if (ptr) ptr++;
            else ptr=new_arg_buff;
#ifdef __WIN32CON__
            strcpy(ptr,fileinfo.cFileName);
#else
            strcpy(ptr,fileinfo.ff_name);
#endif
            result[source].data.text=new_arg_buff;
#ifdef __WIN32CON__
            while (FindNextFile(findhandle,&fileinfo)!=0 && destination<9)
#else
            while (findnext(&fileinfo)==NULL && destination<9)
#endif
            { ptr=result[source].data.text;
              new_arg_buff=store_Xget(strlen(ptr)+16);
              strcpy(new_arg_buff,result[source].data.text);
              ptr=strrchr(new_arg_buff,'\\');
              if (ptr==NULL) ptr=strrchr(new_arg_buff,':');
              if (ptr) ptr++;
              else ptr=new_arg_buff;
#ifdef __WIN32CON__
              strcpy(ptr,fileinfo.cFileName);
#else
              strcpy(ptr,fileinfo.ff_name);
#endif
              result[destination++].data.text=new_arg_buff;
            }
          }
        }
      }
    }
  }
}


/*************************************************
*       Generate names for special keys          *
*************************************************/

/* This procedure is used to generate names for odd key
combinations that are used to represent keystrokes that
can't be generated normally. Return TRUE if the name
has been generated, else FALSE. */

BOOL sys_makespecialname(int key, char *buff)
{ return 0;
}

/*************************************************
*         Additional special key text            *
*************************************************/

/* This function is called after outputting info about
special keys, to allow any system-specific comments to
be included. */

void sys_specialnotes(void)
{;
}


/*************************************************
*  Find a filename somewhere in the current PATH *
*************************************************/

/* Returns pointer to open file, or NULL if not found */

static FILE *find_in_path(char *fname)
{ char *endptr;
  char *ptr;
  char *sys_env_path;
  char fullname[260];
  static FILE *fs;
  sys_env_path = getenv("PATH");
  ptr = sys_env_path;
  while (ptr<sys_env_path+strlen(sys_env_path) && !fs)
  { endptr=strchr(ptr,PATH_DIR_SEPARATOR);
    if (!endptr) endptr=sys_env_path+strlen(sys_env_path);
    strncpy(fullname,ptr,(int)(endptr-ptr));
    fullname[(int)(endptr-ptr)]=PATH_SEPARATOR;
    fullname[(int)(endptr-ptr)+1]=0;
    strcat(fullname,fname);
    fs=fopen(fullname,"r");
    ptr=endptr+1;
  }
  return (fs);
}

/*************************************************
*              Local initialization              *
*************************************************/

/* This is called first thing in main() for doing vital system-
specific early things, especially those concerned with any
special store handling. */

void sys_init1(void)
{ main_tabs = getenv("NETABS");   /* Set default */
  if (main_tabs == NULL) main_tabs = getenv("ETABS");   /* Set default */
  main_backupfiles=TRUE;
#ifdef __WIN32CON__
  OutConsoleHandle=GetStdHandle(STD_OUTPUT_HANDLE);
  InConsoleHandle=GetStdHandle(STD_INPUT_HANDLE);
  GetConsoleScreenBufferInfo(OutConsoleHandle,&OutConsole);
//  scrn_info.screenheight=OutConsole.dwSize.Y;
//  scrn_info.screenwidth=OutConsole.dwSize.X;
  scrn_info.screenheight=OutConsole.srWindow.Bottom-OutConsole.srWindow.Top+1;
  scrn_info.screenwidth=OutConsole.srWindow.Right-OutConsole.srWindow.Left+1;
  scrn_info.normattr=OutConsole.wAttributes;
  SetConsoleMode(InConsoleHandle,ENABLE_ECHO_INPUT+ENABLE_MOUSE_INPUT);
  startrow=OutConsole.dwSize.Y-OutConsole.srWindow.Bottom+OutConsole.srWindow.Top-1;
#else
  gettextinfo(&scrn_info);
  _wscroll=0;
#endif
  scrn_info.attribute =(short)( ((scrn_info.normattr%16*16))+scrn_info.normattr/16);
}

/* This is called after argument decoding is complete to allow
any system-specific over-riding to take place. Main_screenmode
will be TRUE unless -line or -with was specified. */

void sys_init2(void)
{ int keycount = 0;
  char *ptr;
  char helpname[260];

/* find help file - search current directory, then directory from
which .EXE file is launched, then in each directory in the PATH */

  if ((helpfile = fopen(HELP_FILE_NAME,"r")) == NULL)
  { if (strlen(arg_zero))
    { strcpy(helpname,arg_zero);
      if ((ptr = strrchr(helpname,PATH_SEPARATOR)) != NULL) *(ptr + 1) = 0;
      else helpname[0] = 0;
      strcat(helpname,HELP_FILE_NAME);
      helpfile = fopen(helpname,"r");
    }
    if (!helpfile) helpfile = find_in_path(HELP_FILE_NAME);
  }
  main_einit= getenv("NEINIT");   /* Set default */
  if (main_einit == NULL) main_einit = getenv("EINIT");
  if (main_einit)
    if (main_einit[0]=='"' && main_einit[strlen(main_einit)-1]=='"')
    { strcpy(main_einit,main_einit+1);
      main_einit[strlen(main_einit)-1]=0;
    }
  scommon_select();  /* connect to common screen driver */

  key_controlmap    = 0xFFFFFFFEL;    /* exclude nothing */
  key_functionmap   = 0x7FFFFFFEL;    /* allow 1-30 */

/*
bit order is: (ms to ls)
copy key, end, page down, page up, home, ins, tab, return, backspace, del
arrows
*/
  key_specialmap[0] = 0x00001FFFL;
  key_specialmap[1] = 0x00001FFFL;
  key_specialmap[2] = 0x00001FFFL;
  key_specialmap[3] = 0x0L;

  while (non_keys[keycount] != 0)
    key_table[non_keys[keycount++]] = 0;
  key_table[s_f_ins] = 60;
  key_table[s_f_hom] = ka_csls;         /* cursor to (true) line start */
  key_table[s_f_end] = ka_csle;         /* cursor to (true) line end */
  key_table[s_f_tab+sh] = ka_csptab;    /* cursor to previous tab position */
  key_table[s_f_del] = ka_dc;           /* delete character */
  key_table[s_f_del+sh] = ka_cl;        /* closeup */
  key_table[s_f_del+ct] = ka_dar;       /* delete right */
  key_table[s_f_pdn] = ka_scdown;       /* scroll down */
  key_table[s_f_pup] = ka_scup;         /* scroll up */
  key_table[s_f_pup+ct] = ka_sctop;     /* document top*/
  key_table[s_f_pdn+ct] = ka_scbot;     /* document bottom */
  key_table[s_f_cup+ct] = ka_sctop;     /* document top*/
  key_table[s_f_cdn+ct] = ka_scbot;     /* document bottom */

  main_eightbit=TRUE;
  screen_suspend=FALSE;
}



/*************************************************
*                Munge return code               *
*************************************************/

/* This procedure is called just before exiting, to enable a
return code to be set according to the OS's conventions. */

int sys_rc(int rc)
{ return rc;
}


/*************************************************
*             Report local version               *
*************************************************/

/* This procedure is passed the global version string,
and should append to it the version of this local module,
together with the version(s) of any screen drivers. */

void sys_setversion(char *s)
{ strcat(s, this_version);
}


/*************************************************
*             Generate crash file name           *
*************************************************/

char *sys_crashfilename(int which)
{ return which? "NEcrash" : "NEcrash.log";
}


/*************************************************
*              Open a file                       *
*          (ignore file type)                    *
*************************************************/

FILE *sys_fopen(char *name, char *type, int *filetype)
{ char typecpy[3];
  strcpy(typecpy,type);
  if (main_binary) strcat(typecpy,"b");

/* Handle optional automatic backup for output files. We change
extension to .BAK, as is common with MS-DOS. */
  if (main_backupfiles && type[0] == 'w' && !file_written(name))
  { char bakname[260],*dotptr,*slashptr;
    strcpy(bakname, name);
    dotptr = strrchr(bakname,'.');
    slashptr = strrchr(bakname,'\\');
    if (dotptr)
    if(dotptr > slashptr)
      *dotptr = 0;
    strcat(bakname, ".bak");
    remove(bakname);
    rename(name, bakname);
    file_setwritten(name);
  }

  return fopen(name, typecpy);
}


/*************************************************
*              Check file name                   *
*************************************************/

char *sys_checkfilename(char *s, BOOL reading)
{ unsigned char *p = (unsigned char *)s;
#ifdef __WIN32CON__
  unsigned char illegals[]="/*?\"<>|";
#else
  unsigned char illegals[]="*+=[];\",/?<>|";
#endif
  while (*p != 0)
  {
#ifndef __WIN32CON__
    if (*p == ' ')
    {
      while (*(++p) != 0)
      {
        if (*p != ' ') return "(contains a space)";
      }
      break;
    }
#endif
    if (*p < ' ' || *p > '~') return "(contains a non-printing character)";
    p++;
  }
  if ((p=strpbrk(s,illegals))!=NULL)
  {
    static char msg[]="contains illegal character \" \"";
    msg[strlen(msg)-2]=*p;
    return (msg);
  }
  return NULL;

}

/*************************************************
*            Set a file's type                   *
*************************************************/

/* MSDOS does not use file types. */

void sys_setfiletype(char *name, int type)
{ ;
}

/*************************************************
*         Give number of characters for newline  *
*************************************************/

int sys_crlfcount(void)
{ return (2);
}


/*************************************************
*            Write to message stream             *
*************************************************/

/* Output to msgs_fid is passed through here, so that it can
be handled system-specifically - under MS-DOS allows colours. */

void sys_mprintf(FILE *f, char *format, ...)
{
  char outpt[132];
#ifdef __WIN32CON__
  DWORD written;
#endif
  va_list ap;
  va_start(ap, format);
  if (main_screenmode)
  { vsprintf(outpt,format,ap);

#ifdef __WIN32CON__
    GetConsoleScreenBufferInfo(OutConsoleHandle,&OutConsole);
    if (OutConsole.dwCursorPosition.Y>=screen_max_row+startrow)
    {
      if (OutConsole.dwCursorPosition.X<screen_max_col)
      WriteConsole(OutConsoleHandle,  // handle to a OutConsole screen buffer
        outpt,                    // pointer to buffer to write from
        (DWORD) strlen(outpt),    // number of characters to write
        &written,                  // pointer to number of characters written
        NULL                       // reserved
      );
    }
    else
      WriteConsole(OutConsoleHandle,  // handle to a OutConsole screen buffer
        outpt,                    // pointer to buffer to write from
        (DWORD) strlen(outpt),    // number of characters to write
        &written,                  // pointer to number of characters written
        NULL                       // reserved
      );
#else
    cprintf("%s",outpt);
#endif
  }
  else vfprintf(f, format, ap);
  va_end(ap);
}

/*************************************************
*            Give reason for key refusal         *
*************************************************/

/* This procedure gives reasons for the setting of bits
in the maps that forbid certain keys being set. */

char *sys_keyreason(int key)
{ switch (key & ~(s_f_shiftbit+s_f_ctrlbit))
  { case s_f_ret: return "return is equivalent to ctrl/m";
    default:      return "not available on keyboard";
  }
}


/*************************************************
*           System-specific help info            *
*************************************************/

BOOL sys_help(char *s)
{ BOOL show=FALSE, shown=FALSE;
  char helpline[128],lastline[128];
  int linelen;
  if (helpfile)
  { if (!s)
    {/* error_printf("Use, e.g. 'help a' for help items beginning 'a'\n");*/
      fseek(helpfile, 0L, SEEK_SET);
      linelen = 0;
      lastline[0]=0;
      error_printf("Help is available on: \n\n");
      while (fgets(helpline,128,helpfile))
      { if (strlen(helpline))
          if (helpline[0]>' ')
          { if (strchr(helpline,' ')) *(strchr(helpline,' '))=0;
            else if (strchr(helpline,'\n')) *(strchr(helpline,'\n'))=0;
            if (strcmp(helpline,lastline))
            { if (linelen>0) error_printf(", ");
              linelen+=strlen(helpline)+2;
              if (linelen>screen_max_col-1)
              { error_printf("\n");
                linelen=strlen(helpline)+2;
              }
              error_printf("%s", helpline);
            }
            strcpy(lastline,helpline);
          }
        }
        if (linelen>0) error_printf("\n");
        error_printf("\nUse:\n\nHELP <topic>\n\nwhere <topic> can be an abbreviation.\n");
        shown=TRUE;
      }
      else
      { fseek(helpfile, 0L, SEEK_SET);
        while (fgets(helpline,128,helpfile))
        { if (strlen(helpline))
          if (helpline[0]>' ')
            if (!strnicmp(helpline,s,strlen(s))) show=shown=TRUE;
            else show=FALSE;
          if (show) error_printf("%s",helpline);
          if (strlen(s)==1) show = FALSE;
        }
      }
    if (!shown) error_printf("** Sorry, no help available on %s\n", s);
    return TRUE;
  }
  else return FALSE;
}

/* This is called from main_interrupted(). This in turn is called only
when the main part of NE is in control, not during screen editing operations.
We have to check for ^C by steam, as we are running in raw terminal mode.

It turns out that kbhit can be very slow, so we don't actually want
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
{ if (!main_escape_pressed && main_screenmode &&
      (main_cicount & ci_masks[type]) == 0)
  { if (kbhit())
      if (getch() == 3)
      { main_escape_pressed = TRUE;
        main_repaint=TRUE;
      }
  }
}



/*************************************************
*           Tidy up after interrupted fgets      *
*************************************************/

void sys_after_interrupted_fgets(BOOL buffergiven, char *buffer)
{;
}

/*************************************************
*              Accept keystroke                  *
*************************************************/

void sys_keystroke(int key)
{ if (setjmp(error_jmpbuf) == 0)
  { error_longjmpOK = TRUE;
    if (key && (key < 32)) key_handle_function(key);
    else if (key== 0)
    { key=getachar();
      key_handle_function(ControlKeystroke[key-14]);
    }
    else if (key== 127) key_handle_function(ControlKeystroke[0]);
    else key_handle_data(key);
  }
  error_longjmpOK = FALSE;
}



/*************************************************
*       Get keystroke for command line           *
*************************************************/

/* This function is called when reading a command line in screen
mode. It must return a type and a value. */

int sys_cmdkeystroke(int *type)
{ int key = nextchar(type);
  if (*type == -1)
  { if (key && key < 32) *type = ktype_ctrl;
   else
    if (key == 0)
    { key = getachar();
      *type = ktype_function;
      key = ControlKeystroke[key-14];
      if (key==s_f_ret) key='\r';
    }
    else if (key == 127)
    { *type = ktype_function;
      key = ControlKeystroke[0];
      }
      else *type = ktype_data;
  }
  return key;
}

/*************************************************
*         Position to screen bottom for crash    *
*************************************************/

/* This function is called when about to issue a disaster error message.
It should position the cursor to a clear point at the screen bottom, and
reset the terminal if necessary. */

void sys_crashposition(void)
{ pc_rendition(s_r_normal);
  pc_move(0, screen_max_row);
  main_screenmode = FALSE;
}

/*************************************************
*       Entry point for non-window screen run    *
*************************************************/

void sys_runscreen(void)
{ FILE *fid = NULL;
  char *fromname = arg_from_name;
  char *toname = (arg_to_name == NULL)? arg_from_name : arg_to_name;
  int ftype;
  sys_w_cls = pc_cls;
  sys_w_move = pc_move;
  sys_w_rendition = pc_rendition;
  sys_w_putc = pc_putc;

  sys_setupterminal = setupterminal;
  sys_resetterminal = NULL;

  sys_w_hscroll = NULL;

  sys_w_vscroll = pc_vscroll;
  sys_w_flush = pc_flush;
  arg_from_name = arg_to_name = NULL;
  currentbuffer = main_bufferchain = NULL;
  setupterminal();

/* If a file name is given, check for its existence before setting up
   the screen. */

  if (fromname != NULL && strcmp(fromname, "-") != 0)
  { fid = sys_fopen(fromname, "r", &ftype);
    if (fid == NULL)
    { printf("** The file \"%s\" could not be opened.\n", fromname);
      printf("** NE abandoned.\n");
      main_rc = 16;
      return;
    }
  }

/* Set up the screen and cause windows to be defined */

  s_init(screen_max_row, screen_max_col, TRUE);
  scrn_init(TRUE);
  scrn_windows();                     /* cause windows to be defined */
  main_screenOK = TRUE;

  if (init_init(fid, fromname, 0, toname, main_stream))
  { int x, y;

    if (!main_noinit && main_einit != NULL) cmd_obey(main_einit);
    main_initialized = TRUE;
    if (main_opt != NULL)
    { int yield;
      scrn_display(FALSE);
      s_selwindow(message_window, 0, 0);
      s_rendition(s_r_normal);
      s_flush();
      yield = cmd_obey(main_opt);
      if (yield != done_continue && yield != done_finish)
      { //scrn_hint(sh_redraw, 0, NULL);
        screen_forcecls=TRUE;
        scrn_rdline(TRUE, FALSE, "Press RETURN to continue ");
      }
      if (yield==done_finish) return;
    }
    if (!main_done)
    { scrn_display(FALSE);
      x = s_x();
      y = s_y();

      s_selwindow(message_window, 0, 0);
      s_printf("NE version %s %s", version_string, version_date);
      main_shownlogo = TRUE;
      s_selwindow(first_window, x, y);
#ifndef __WIN32CON__
      setcbrk(0);
#endif
    }
    while (!main_done)
    { int type;
      unsigned c = nextchar(&type);

      main_rc = error_count = 0;           /* reset at each interaction */
      if (type < 0)
        sys_keystroke(c);
      else
        key_handle_data(c);
    }
  }
}

static int nextchar(int *type)
{ int c;
  c = getachar();               /* Get next key */
  *type = -1;                   /* unset - set only for forced data */
  if (c == 27)
  { ESCAPED=TRUE;
    c = getachar();
    *type = ktype_data;
    if (c == 0) c = getachar();
  }
  else ESCAPED=FALSE;
  return c;
}

#ifdef __WIN32CON__
// simplify code a little
#define keyflags keystroke.Event.KeyEvent.dwControlKeyState
#define scancode keystroke.Event.KeyEvent.wVirtualScanCode
#define ascii keystroke.Event.KeyEvent.uChar.AsciiChar
#else
/* Use enhanced keyboard codes, if available */
#define ENHANCED_KB_FLAG ((char far *)(0x400096L))
#endif
static int getachar(void)
{ static int flag;
  static unsigned ch;
#ifdef __WIN32CON__
  DWORD nkeystrokes;
  struct _INPUT_RECORD keystroke;
  unsigned char shiftkeys[]={0x1d,0x2a,0x36,0x38,0x3a,0x45,0x46,0x00};
  unsigned char ctrls[]={0x77,0x8d,0x84,0x4a,0x73,0x4c,0x74,0x4e,0x75,0x91,0x76,0x92,0x93,0x00};
  unsigned char alts[]={0x97,0x98,0x99,0x4a,0x9b,0x4c,0x9d,0x4e,0x9f,0xa0,0xa1,0xa2,0xa3,0x00};
#else
  static int keyflags;
#endif
  if (flag !=-1)
#ifndef __WIN32CON__
  { ch = bioskey(*(ENHANCED_KB_FLAG) & 0x10);
    keyflags = (bioskey(2+ (*(ENHANCED_KB_FLAG) & 0x10))) & 0xf;
#else
  { do
    { ReadConsoleInput(
        InConsoleHandle,  // handle to a console input buffer
        &keystroke,  // pointer to the buffer for peek data
        (DWORD) 1,  // number of records to read
        &nkeystrokes   // pointer to number of records read
      );
      if (keystroke.EventType==MOUSE_EVENT)
      { if ((keystroke.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED))
        { int i;
          if (keystroke.Event.MouseEvent.dwMousePosition.Y-startrow>0
              && keystroke.Event.MouseEvent.dwMousePosition.Y-startrow<=window_bottom)
          { cursor_row=keystroke.Event.MouseEvent.dwMousePosition.Y-startrow;
            if (cursor_row>lastrow)
              for (i=lastrow;i<cursor_row;i++)
              { if (main_current->next!=NULL)
                  main_current=main_current->next;
              }
            else
              for (i=lastrow;i>cursor_row;i--)
              { if (main_current->prev!=NULL)
                  main_current=main_current->prev;
              }
            main_currentlinenumber=cursor_row;
            cursor_col=keystroke.Event.MouseEvent.dwMousePosition.X;
            pc_move(cursor_col,cursor_row);
            break;
          }
        }
      }
    } while (strchr(shiftkeys,scancode)
             || nkeystrokes==0
             || keystroke.EventType!=KEY_EVENT
             ||keystroke.Event.KeyEvent.bKeyDown==0
      );
//    debug_printf("scancode=%8x,keyflags=%8x,ascii=%8x\n",scancode,keyflags,ascii);
    ch=(scancode<<8)+(ascii & 0xff);
    if (scancode>=0x3b)
    { if (keyflags!=0)
      { if (scancode<=0x44)
          ch+=(0x1900*((keyflags & SHIFT_PRESSED)>0)
               +0x2300*((keyflags & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))>0));
        if (scancode>=0x47 && scancode<=0x53)
        { if ((keyflags & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))>0)
            ch=(ctrls[(scancode)-0x47]<<8)+(ascii & 0xff);
          else
            if ((keyflags & (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))>0)
            ch=(alts[(scancode)-0x47]<<8)+(ascii & 0xff);
        }
      }
    }
//    if (strchr(shiftkeys,scancode)) debug_printf("\n");
    if (!flag && ((keyflags & LEFT_ALT_PRESSED) || (keyflags & RIGHT_ALT_PRESSED)))
      ch &= 0xff00;
#endif
    if (ESCAPED)
      ESCAPED=FALSE;
    else
    { if ((ch & 0xff00) == 0x0e00)
      { ch = 0x0e00;                    // backsp as scan
#ifdef __WIN32CON__
        if ((keyflags & SHIFT_PRESSED)>0)
#else
        if (keyflags > 0 && keyflags < 4)
#endif
          ch=0xb400;
#ifdef __WIN32CON__
        if ((keyflags & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))>0)
#else
        if (keyflags == 4)
#endif
         ch=0xb600;
      }
      if ((ch & 0xff00) == 0x0f00)
      { ch = 0x0f00;                            // tab as scan
#ifdef __WIN32CON__
        if ((keyflags & SHIFT_PRESSED)>0)
#else
        if (keyflags > 0 && keyflags < 4)
#endif
          ch=0xb500;
#ifdef __WIN32CON__
        if ((keyflags & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))>0)
#else
        if (keyflags == 4)
#endif
          ch=0xb700;
      }
      if ((ch & 0xff00) == 0x1c00)
      { ch = 0x1c00;                            // ret as scan
#ifdef __WIN32CON__
        if ((keyflags & SHIFT_PRESSED)>0)
#else
        if (keyflags > 0 && keyflags < 4)
#endif
          ch=0xb800;
#ifdef __WIN32CON__
        if ((keyflags & (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))>0)
#else
        if (keyflags == 4)
#endif
          ch=0xb900;
      }
    }
    if ((ch < 0x5400) && (ch > 0x46e0) && (ch & 0xff)==0xe0
#ifdef __WIN32CON__
        && (keyflags & SHIFT_PRESSED)>0)
#else
        && keyflags > 0 && keyflags < 4)
#endif
      ch = ch + 0x6000;
    if ((ch & 0xff) == 0xe0) ch = ch & 0xff00;
    if ((ch & 0xff) == 0) flag = -1;
    else flag = 0;
    return(ch & 0xff);
  }
  else
  { flag = 0;
    return ((ch >> 8) & 0xff);
  }
}

static void pc_cls(int bottom, int left, int top, int right)
{ int i;
  struct text_info info;
#ifdef __WIN32CON__
  struct _COORD scrn;
  DWORD result;
  WORD *buffer;
  char *sbuffer;
  GetConsoleScreenBufferInfo(OutConsoleHandle,&OutConsole);
//  info.screenheight=OutConsole.dwSize.Y;
//  info.screenwidth=OutConsole.dwSize.X;
  info.screenheight=OutConsole.srWindow.Bottom-OutConsole.srWindow.Top+1;
  info.screenwidth=OutConsole.srWindow.Right-OutConsole.srWindow.Left+1;
  info.normattr=OutConsole.wAttributes;
  info.attribute = (unsigned short)(((info.normattr%16*16) & 127)+info.normattr/16);
  info.winleft=(unsigned short)(OutConsole.srWindow.Left+1);
  info.wintop=(unsigned short)(OutConsole.srWindow.Top+1);
  info.winright=(unsigned short)(OutConsole.srWindow.Right+1);
  info.winbottom=(unsigned short)(OutConsole.srWindow.Bottom+1);
  scrn.X=(short)left;
  if ((buffer=calloc(info.winright+4,sizeof(WORD)))==NULL)
  { printf ("error cls\n");
    exit (99);
  }
  if ((sbuffer=calloc(info.winright+4,sizeof(WORD)))==NULL)
  { printf ("error cls1\n");
    exit (99);
  }
  for (i=0;i<=info.winright;i++) buffer[i]=info.normattr;
  for (scrn.Y=(short)top+startrow;scrn.Y<=bottom+startrow;scrn.Y++)
    WriteConsoleOutputAttribute(
      OutConsoleHandle,          // handle to a OutConsole screen buffer
      buffer,                  // pointer to buffer to write attributes from
      (DWORD)(right-left+1),  // number of character cells to write to
      scrn,                    // coordinates of first cell to write to
      &result                 // pointer to number of cells written to
     );
  for (i=0;i<=info.winright;i++) sbuffer[i]=' ';
  for (scrn.Y=(short)top+startrow;scrn.Y<=bottom+startrow;scrn.Y++)
    WriteConsoleOutputCharacter(
      OutConsoleHandle,          // handle to a OutConsole screen buffer
      sbuffer,                  // pointer to buffer to write characters from
      (DWORD)(right-left+1),  // number of character cells to write to
      scrn,                    // coordinates of first cell to write to
      &result                  // pointer to number of cells written to
   );
  free(buffer);
free(sbuffer);
#else
  gettextinfo(&info);
  window(left+1,top+1,right+1,bottom+1);
  clrscr();
  window(info.winleft,info.wintop,info.winright,info.winbottom);
#endif
}

static void pc_move(int x, int y)
{
#ifdef __WIN32CON__
  cursorpos.X=(short)x;
  cursorpos.Y=(short)y+startrow;
  SetConsoleCursorPosition(OutConsoleHandle,cursorpos);
  lastrow=y;
  lastcol=x;
#else
  gotoxy(x+1,y+1);
#endif
}

static void pc_rendition(int r)
{ if (r==s_r_normal)
#ifdef __WIN32CON__
    SetConsoleTextAttribute(OutConsoleHandle,(WORD) scrn_info.normattr);
#else
    textattr(scrn_info.normattr);
#endif
  else
#ifdef __WIN32CON__
    SetConsoleTextAttribute(OutConsoleHandle,(WORD) scrn_info.attribute);
#else
    textattr(scrn_info.attribute);
#endif
}


static void pc_putc(int c)
{
#ifdef __WIN32CON__
  strchr("\7\b\n\r\t",c) ? sys_mprintf(msgs_fid,"%c",0xfe): sys_mprintf(msgs_fid,"%c",c);
#else
  strchr("\7\b\n\r",c) ? sys_mprintf(msgs_fid,"%c",0xfe): sys_mprintf(msgs_fid,"%c",c);
#endif
}

/*
static void pc_hscroll(int left, int bottom, int right, int top, int amount)
{ ;
}
*/
#ifdef __WIN32CON__
int movetext(int left, int top, int right, int bottom, int destleft, int desttop)
{ SMALL_RECT Region;
  COORD BufferSize, BufferCoord;
  CHAR_INFO *buffer;
  if ((buffer=calloc((bottom-top+1)*(right-left+1),sizeof(CHAR_INFO)))==NULL)
  { printf ("error movetext\n");
    exit (99);
  }
  BufferSize.X=(short)(right-left+1);
  BufferSize.Y=(short)(bottom-top+1);
  BufferCoord.X=0;
  BufferCoord.Y=0;
  Region.Left=(short)(left-1);
  Region.Top=(short)(top-1+startrow);
  Region.Right=(short)(right-1);
  Region.Bottom=(short)(bottom-1+startrow);
  ReadConsoleOutput(
     OutConsoleHandle,       //HANDLE hConsoleOutput,  // handle of a console screen buffer
     buffer,                 // PCHAR_INFO lpBuffer,  // address of buffer that receives data
     BufferSize,             //COORD dwBufferSize,  // column-row size of destination buffer
     BufferCoord,            //COORD dwBufferCoord,  // upper-left cell to write to
     &Region                 //PSMALL_RECT lpReadRegion   // address of rectangle to read from
   );
  Region.Left=(short)(destleft-1);
  Region.Top=(short)(desttop-1+startrow);
  Region.Right=(short)(Region.Left+right-left);
  Region.Bottom=(short)(Region.Top +bottom-top+startrow);
  WriteConsoleOutput(
     OutConsoleHandle,       //HANDLE hConsoleOutput,  // handle of a console screen buffer
     buffer,                 // PCHAR_INFO lpBuffer,  // address of buffer that receives data
     BufferSize,             //COORD dwBufferSize,  // column-row size of destination buffer
     BufferCoord,            //COORD dwBufferCoord,  // upper-left cell to write to
     &Region                 //PSMALL_RECT lpReadRegion   // address of rectangle to read from
   );
  free(buffer);
  return(0);
}
#endif

static void pc_vscroll(int bottom, int top, int amount, BOOL deleting)
{ int i,j;
  if (amount<0)
  { movetext(1,top+1-amount,screen_max_col+1,bottom+1,1,top+1);
    for (i=bottom;i>bottom+amount;i--)
    { pc_move(0,i);
      for (j=0;j<=screen_max_col;j++) pc_putc(' ');
//      clreol();
    }
  }
  else
  { movetext(1,top+1,screen_max_col+1,bottom-amount+1,1,top+1+amount);
      for (i=top;i<top+amount;i++)
      { pc_move(0,i);
         for (j=0;j<=screen_max_col;j++) pc_putc(' ');
//         clreol();
      }
  }
}

/*************************************************
*                  Set screen                    *
*************************************************/

static void setupterminal(void)
{ char *ne_colour,colr_norm, colr_rev;
  colr_norm=scrn_info.normattr;
  colr_rev=scrn_info.attribute;
  screen_max_row = scrn_info.screenheight-1;
  screen_max_col = default_rmargin = scrn_info.screenwidth-1;
  if (colour_set)
  { scrn_info.normattr=colr_norm;
    scrn_info.attribute=colr_rev;
  }
  else
    scrn_info.attribute =(short)( ((scrn_info.normattr%16*16))+scrn_info.normattr/16);
  ne_colour = getenv("NECOL");        /* get any colour setting */
  if (!colour_set && ne_colour)
    setupcolours(ne_colour);
  pc_rendition(s_r_normal);
}

/*************************************************
*                  Unset terminal state          *
*************************************************/


/*static void resetterminal(void)
{ ;
}
*/

static void pc_flush(void)
{
#ifdef __WIN32CON__
    SetConsoleTextAttribute(OutConsoleHandle,(WORD) scrn_info.normattr);
#else
    textattr(scrn_info.normattr);
#endif
}

void  sys_hourglass(BOOL onoff)
{ ;
}

