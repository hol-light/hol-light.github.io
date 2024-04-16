/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1999 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: November 1999 */


/* This file contains the system-specific routines for the
RISC OS operating system that runs on Acorn Archimedes computers,
with the exception of the window-specific code, which lives in
its own modules. */


#include "ehdr.h"
#include "roshdr.h"
#include "scomhdr.h"
#include "keyhdr.h"


#define this_version "0.34"


/* List of signals that are to be trapped, causing an abort
of E and dump of the buffers. The names are also given, for
use in the message. */

int signal_list[] = {
  SIGABRT, SIGFPE,  SIGILL, SIGSEGV,
  SIGTERM, SIGSTAK, SIGOSERROR, -1 };

char *signal_names[] = {
  "(SIGABRT)", "(SIGFPE)",  "(SIGILL)", "(SIGSEGV)",
  "(SIGTERM)", "(SIGSTAK)", "(SIGOSERROR)", "" };
  

/*************************************************
*          Globals for RISC OS routines          *
*************************************************/

/* Those that are specifically for sros are held therein. */

BOOL hourglass_on = FALSE;
  


/*************************************************
*          Local store handling routines         *
*************************************************/

/* When windowing, we use the xs system to get store above the program. 
Individual calls to free are ignored; these only occur when the whole 
thing is going to be reset anyway. */

void *sys_Malloc(int size)
{
return main_windowing? xs_get(size) : malloc(size);
} 

void sys_Free(void *p)
{
if (!main_windowing) free(p);
}

void sys_Free_all(void)
{
if (main_windowing) xs_free_all();
}



/*************************************************
*          Call SWI and return error             *
*************************************************/

OSerror *ESWI(int swi, int count, ...)
{
int i;
va_list ap;
va_start(ap, count);
for (i = 0; i < count; i++) swi_regs.r[i] = va_arg(ap, int);
va_end(ap);
return _kernel_swi(swi, &swi_regs, &swi_regs);
}




/*************************************************
*              Control hourglass                 *
*************************************************/

void sys_hourglass(BOOL on)
{
if (main_windowing)
  {
  SWI(on? Hourglass_On : Hourglass_Off, 0);
  hourglass_on = on;
  } 
}



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
*              Local initialization              *
*************************************************/

/* This is called first thing in main() for doing vital system-
specific early things, especially those concerned with any
special store handling. */

void sys_init1(void)
{
char *tw = getenv("NE$TaskWindow");
if (tolower(tw[0]) == 'n' && tolower(tw[1]) == 'o' && tolower(tw[2]) == 0)
  support_taskwindows = FALSE;
if (main_windowing) xs_init();
main_tabs = getenv("NE$Tabs");                            /* default tab option */
if (main_tabs == NULL) main_tabs = getenv("E$Tabs");      /* default tab option */
default_filetype = 0xfff;                                 /* default is "text" */
sys_varname_NEINIT = "NE$INIT ";                          /* for -help output */
sys_varname_NETABS = "NE$TABS ";
}


/* This is called after argument decoding is complete to allow
any system-specific over-riding to take place. Main_screenmode
will be TRUE unless -line or -with was specified. */

void sys_init2(void)
{
char *init = getenv("NE$Init");
if (init == NULL) init = getenv("E$Init");

/* We must save main_einit, as the store pointed to by getenv is permitted
to change, and this may last a long time. In particular, calls to system()
seem to change it. */

if (init != NULL)       /* main_einit defaults to NULL */
  {
  main_einit = malloc(strlen(init) + 1);
  strcpy(main_einit, init);
  }

/* Run line-by-line in a task window */

if (SWI(TaskWindow_Taskinfo, 1, 0) != 0) main_screenmode = FALSE;

/* Else set up screen parameters */

else
  {  
  SWI(OS_ReadModeVariable, 2, -1, 1);
  mode_max_col = screen_max_col = swi_regs.r[2];
  SWI(OS_ReadModeVariable, 2, -1, 2);
  mode_max_row = screen_max_row = swi_regs.r[2];
  } 

main_eightbit = TRUE;     /* display top-bit-set characters */
topbit_minimum = 140;     /* starting from here */
screen_suspend = FALSE;   /* no need to suspend for * commands */
scommon_select();         /* connect to common screen driver */

key_controlmap    = 0xFFFFFFFE;    /* exclude nothing */
key_functionmap   = 0x7FFFFFFE;    /* allow 1-30 */
key_specialmap[0] = 0x000021BF;    /* copy, insert, tab, bsp, del, arrows */
key_specialmap[1] = 0x000021BF;    /* copy, insert, tab, bsp, del, arrows */
key_specialmap[2] = 0x000021BF;    /* copy, insert, tab, bsp, del, arrows */
key_specialmap[3] = 0x000001BF;    /*       insert, tab, bsp, del, arrows */
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
*           Yield crash file names               *
*************************************************/

char *sys_crashfilename(int which)
{
return which? "NEcrash" : "NEcrashLog";
}


/*************************************************
*                Open a file                     *
*************************************************/

/* On some systems, the name gets munged. Here, we check for a
directory, to be tidy. */

FILE *sys_fopen(char *name, char *type, int *filetype)
{
sys_openfail_reason = of_other;

if (name[0] != 0 && name[strlen(name)-1] != ':')
  {
  int rc = SWI(OS_File, 2, 5, (int)name);
  if (rc == 2)
    {
    BOOL oldinit = main_initialized;
    if (main_windowing) main_initialized = TRUE;     /* Prevent abandonment */
    error_moan(49, name, "it is a directory"); 
    main_initialized = oldinit;
    return NULL; 
    }
 
  if (rc != 1) sys_openfail_reason = of_existence;
  } 
   
if (filetype != NULL)
  {
  *filetype = 0xfff;                                 /* default is "text" */
  if (SWI(OS_File, 2, 5, (int)name) == 1)
    {
    if ((swi_regs.r[2] & 0xFFF00000) == 0xFFF00000)  /* Indicates typed file */
      *filetype = (swi_regs.r[2] & 0x000FFF00) >> 8;
    }
  }     

/* Handle optional automatic backup for output files; not conventional
on RISC OS, but some people might like it, and this code is in any case
for testing the infrastructure. We either add "~" to the name, or change 
the last character to "~" if the final component is 10 characters long. */

if (main_backupfiles && strcmp(type, "w") == 0 && !file_written(name))
  {
  char *p, *pp; 
  char bakname[80];
  strcpy(bakname, name);
  pp = p = bakname + strlen(name);
  while (--p >= bakname && *p != '.');
  if (pp - p >= 10) pp--;
  *pp++ = '~';
  *pp = 0;   
  remove(bakname);
  rename(name, bakname);
  file_setwritten(name);    
  } 
 
return fopen(name, type);
}


/*************************************************
*           Set file type                        *
*************************************************/

void sys_setfiletype(char *name, int type)
{
char buff[100];
sprintf(buff, "settype %s %x\r\n", name, type);
system(buff);
}


/*************************************************
*              Check file name                   *
*************************************************/

/* Use the OS to check the name, but we must check for embedded
control characters or spaces ourselves, as they terminate the
name given to the OS. We allow trailing spaces. */

char *sys_checkfilename(char *s, BOOL reading)
{
OSerror *error;
unsigned char *p = (unsigned char *)s;

while (*p != 0)
  {
  if (*p == ' ')
    {
    while (*(++p) != 0)
      {
      if (*p != ' ') return "contains a space";
      }
    break;
    }
  if (*p < ' ' || *p > '~') return "contains a non-printing character";
  p++;
  }

/* Attempt to read catalog information for file. The
SWI will give an error only on bad name, etc. "Not found"
does not cause the SWI to fail (it just gives R0=0). */

error = ESWI(8, 2, 17, (int)s);
return (error == NULL)? NULL : error->errmess;
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
if (key == s_f_copykey+s_f_shiftbit+s_f_ctrlbit)
  return "key is used for binary data input";

switch (key & ~(s_f_shiftbit+s_f_ctrlbit))
  {
  case s_f_bsp: return "backspace is equivalent to ctrl/h";
  case s_f_ret: return "return is equivalent to ctrl/m";
  case s_f_hom: return "home is equivalent to ctrl/^";
  case s_f_pup: return "page up is equivalent to shift/up";
  case s_f_pdn: return "page down is equivalent to shift/down";
  default:      return "not available on keyboard";
  }
}


/*************************************************
*     Generate name for special keystrokes       *
*************************************************/

/* This procedure is used to generate names for odd key
combinations that are mapped into the more general set
of keys. Under RISC OS there are none; return FALSE and
the general code will give the general name. */

BOOL sys_makespecialname(int key, char *buff)
{
return FALSE;
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
*     System-specific test for interruption      *
*************************************************/

/* Nothing is required here for RISC OS. */

void sys_checkinterrupt(int type)
{
type = type;
}


/*************************************************
*         Count of characters for line end       *
*************************************************/

/* Specify how many characters are in a file to represent
the end of a line. */

int sys_crlfcount(void)
{
return 1;
}


/*************************************************
*       Output system-specific key comments      *
*************************************************/

/* This function is called after info about special keys
has been output, to allow any system-specific comments
to be added. */

void sys_specialnotes(void)
{
}



/*************************************************
*        Clean up after interrupted fgets        *
*************************************************/

/* This function is called twice after a SIGINT interrupted an
fgets() call in line-by-line mode. The first time is immediately
after the longjmp, with buffergiven = FALSE and buffer = NULL. The
second is after the next fgets, with buffergiven = TRUE, and buffer
pointing to the fgotten buffer. This should give most systems a
chance to clean up. In RISC OS one gets a zero byte at the start
of the next buffer. (Trying to read it in the first call gives
other problems. This area is a mess.) */

void sys_after_interrupted_fgets(BOOL buffergiven, char *buffer)
{
if (buffergiven && buffer[0] == 0) buffer[0] = ' ';
}


/* End of sysros.c */
