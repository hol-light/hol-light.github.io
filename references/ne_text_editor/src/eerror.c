/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1997 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: December 1997 */


/* This file contains code for handling errors */

#include "ehdr.h"
#include "shdr.h"

#define rc_noerror   0
#define rc_warning   4
#define rc_serious   8
#define rc_failed   12
#define rc_disaster 16


/* Buffer for the use of error_printf */


static char buff[256];
static int  buffptr = 0;




/*************************************************
*            Texts and return codes              *
*************************************************/

typedef struct {
  char rc;
  char showcmd;
  char *text;
} error_struct;


static error_struct error_data[] = {

/* 0-4 */
{ rc_disaster, FALSE, "Failed to decode command line: \"%s\" %s\n" },
{ rc_disaster, FALSE, "Ran out of store: %d bytes unavailable\n" },
{ rc_disaster, FALSE, "Internal failure - store overlap upwards (%x %d %x)\n" },
{ rc_disaster, FALSE, "Internal failure - store overlap downwards (%x %d %x)\n" },
{ rc_disaster, FALSE, "Internal failure - %s in %s (%d, %d, %d, %d, %d)\n" },
/* 5-9 */
{ rc_serious,  FALSE, "Failed to open file \"%s\"\n" },
{ rc_disaster, FALSE, "Internal failure - current line pointer is NULL\n" },
{ rc_serious,  TRUE,  "Unmatched closing bracket\n" },
{ rc_serious,  TRUE,  "Semicolon expected (characters after command end)\n" },
{ rc_serious,  TRUE,  "Unexpected ELSE (prematurely terminated IF or UNLESS?)\n" },
/* 10-14 */
{ rc_serious,  TRUE,  "Unknown command \"%s\"\n" },
{ rc_serious,  FALSE, "Unexpected response (\"to\" is required before a file name)\n" },
{ rc_serious,  FALSE, "Illegal filename \"%s\" (%s)\n" },
{ rc_serious,  TRUE,  "%s expected\n" },
{ rc_serious,  TRUE,  "Error in key definition string (at character %ld):\n%s\n" },
/* 15-19 */
{ rc_serious,  FALSE, "%s not allowed %s\n" },
{ rc_serious,  FALSE, "No previous %s\n" },
{ rc_serious,  FALSE, "%s not found\n" },
{ rc_serious,  TRUE,  "Error in hexadecimal string: %s\n" },
{ rc_serious,  TRUE,  "Error in hexadecimal string at character %d: %s\n" },
/* 20-24 */
{ rc_serious,  TRUE,  "Repeated or incompatible qualifier\n" },
{ rc_serious,  TRUE,  "Only %s qualifiers allowed on insertion strings for this command\n" },
{ rc_serious,  TRUE,  "n, s, u and w are the only qualifiers allowed with a search expression\n" },
{ rc_serious,  FALSE, "Keyboard interrupt\n" },
{ rc_warning,  FALSE, "The contents of buffer %d have not been saved\n" },
/* 25-29 */
{ rc_serious,  TRUE,  "Line %d not found\n" },
{ rc_serious,  FALSE, "Buffer %d does not exist\n" },
{ rc_serious,  TRUE,  "The B, E, or P qualifier is required for an empty string in a global command\n" },
{ rc_warning,  FALSE, "The contents of the cut buffer have not been pasted.\n" },
{ rc_serious,  FALSE, "Unexpected %s in %s command\n" },
/* 30-34 */
{ rc_serious,  FALSE, "Unexpected %s while obeying \"%s\" command\n" },
{ rc_serious,  FALSE, "Procedure calls too deeply nested\n" },
{ rc_serious,  FALSE, "Unexpected end of file while reading NE commands\n" },
{ rc_serious,  TRUE,  "Missing second argument for \"%s\" command\n" },
{ rc_serious,  TRUE,  "Incorrect value for %s (%s)\n" },
/* 35-39 */
{ rc_serious,  TRUE,  "Function key number not in range 1-%d\n" },
{ rc_disaster, FALSE, "Sorry! NE has crashed on receiving signal %d %s\n" },
{ rc_serious,  FALSE, "I/O error while writing file \"%s\": %s\n" },
{ rc_serious,  TRUE,  "Error in regular expression (at character %d):\n   %s\n" },
{ rc_serious,  FALSE, "Cannot extract wild strings - regular expression match too complicated\n" },
/* 40-44 */
{ rc_serious,  FALSE, "Cursor must be at line start for whole-line change\n" },
{ rc_serious,  FALSE, "No mark set for %s command\n" },
{ rc_serious,  FALSE, "Sorry, no help%s%s is available\n"
                      "(Use \"show keys\" for keystroke information)\n" },
{ rc_serious,  FALSE, "%s mark already set\n" },
{ rc_serious,  TRUE,  "Error in argument for \"word\" (at character %d): %s\n" },
/* 45-49 */
{ rc_serious,  TRUE,  "Procedure %s already exists\n" },
{ rc_serious,  TRUE,  "Malformed procedure name (must be \'.\' followed by letters or digits\n" },
{ rc_serious,  FALSE, "Attempt to cancel active procedure %s\n" },
{ rc_serious,  FALSE, "Procedure %s not found\n" },
{ rc_serious,  FALSE, "\"%s\" cannot be opened because %s\n" },
/* 50-54 */
{ rc_serious,  FALSE, "Commands are being read from buffer %d, so it cannot be deleted\n" },
{ rc_serious,  FALSE, "Buffer %d already exists\n" },
{ rc_serious,  FALSE, "The \"%s\" command is not allowed in a read-only buffer\n" },
{ rc_serious,  FALSE, "The current buffer is read-only\n" },
{ rc_serious,  FALSE, "The current buffer is a stream buffer; %s is not allowed\n" },
/* 55-59 */
{ rc_serious,  FALSE, "Unexpected response\n" },
{ rc_serious,  FALSE, "Command line in buffer is too long\n" },
{ rc_serious,  FALSE, "DBUFFER interrupted - lines have been deleted\n" },
{ rc_serious,  FALSE, "Binary file contains \"%c\" where a hex digit is expected\n" },
{ rc_serious,  FALSE, "Output file not specified for buffer %d - not written\n" },
/* 60-64 */
{ rc_serious,  FALSE, "Only one of %s may be the standard %s\n" },
{ rc_serious,  FALSE, "Commands cannot be read from binary buffers\n" },
{ rc_serious,  FALSE, "Internal failure - \"back\" line not found\n" },
{ rc_serious,  TRUE,  "Error in regular expression at offset %d:\n   %s\n" }
};

#define error_maxerror 63


/*************************************************
*            Display error message               *
*************************************************/

/* If running in a window system, "normal" errors are displayed in a
scrollable text window. Serious errors are too, provided there is a
way out via longjmp. If not, we have to use a system-specific window
error display (with confirmation), because the program is going to exit,
and we want the user to see the message first.

Text from successive calls is buffered up until a newline or a call
to error_printflush, because that gives greater efficiency in windowing
systems (well, on the Archimedes, anyway). */

void error_printflush(void)
{
char *CR = (msgs_fid == stdout || msgs_fid == stderr)? "\r": "";

if (buffptr == 0) return;

if (main_logging) debug_writelog("%s", buff);

#ifdef Windowing
if (main_windowing)
  {
  if (!error_werr && (main_rc < rc_failed || error_longjmpOK))
    sys_window_display_text(buff);
      else sys_werr(buff);
  }
else
#endif

  {
  char *p = buff;
  if (main_screenOK)
    {
    screen_forcecls = TRUE;
    if (s_window() != message_window)
      { 
      s_selwindow(message_window, 0, 0);
      s_cls();
      s_flush(); 
      }    
    }   
     
  if (main_pendnl)
    {
    sys_mprintf(msgs_fid, "%s\n", CR);
    main_pendnl = main_nowait = FALSE;
    }

  while (*p)
    {
    if (*p == '\n') sys_mprintf(msgs_fid, CR);
    sys_mprintf(msgs_fid, "%c", *p++); 
    }
  }
buffptr = 0;
}

/* We would like to use ANSI vsprintf below, but it is one of
the main incompatibilities with non-ANSI libraries, so perhaps
it is best avoided. Sigh. */

void error_printf(char *format, ...)
{
va_list ap;
va_start(ap, format);

/* If the last thing that was output was a line verification with
a non-null pointer, we need to output a newline. */

if (main_verified_ptr)
  {
  main_verified_ptr = FALSE;
  error_printf("\n");
  }

/***
buffptr += vsprintf(buff + buffptr, format, ap);
***/
vsprintf(buff + buffptr, format, ap);
buffptr = strlen(buff);
if (buffptr > 200 || buff[buffptr-1] == '\n') error_printflush();
va_end(ap);
}


/*************************************************
*            Generate error message              *
*************************************************/

void error_moan(int n, ...)
{
int rc, orig_rc;
char buff[256];
va_list ap;
va_start(ap, n);

/* Show logo if not shown, for non-windowing operation. In a non-
windowing screen operation, it will have been shown at the start. */

#ifdef Windowing
if (!main_shownlogo && !main_windowing)
#else
if (!main_shownlogo)
#endif
  {
  error_printf("NE version %s %s\n", version_string, version_date);
  main_shownlogo = TRUE;
  }

/* Set up the error text */

if (n > error_maxerror)
  {
  sprintf(buff, "** Unknown error number %d", n);
  orig_rc = rc = rc_disaster;
  }
else
  {
  strcpy(buff, "** ");
  vsprintf(buff + 3, error_data[n].text, ap);
  orig_rc = rc = error_data[n].rc;
  }
va_end(ap);

/* If not yet fully initialized, all errors are disasters */

if (!main_initialized) rc = rc_disaster;

/* If too many errors, replace the text and force a high return code */

if (rc > rc_warning && ++error_count > 40)
  {
  if (rc < rc_failed) rc = rc_failed;
  strcpy(buff, "** Too many errors\n");
  }

/* Set the return code for the run */

if (rc > main_rc) main_rc = rc;

/* We cannot print/display anything before this, because the display/print
routine needs to know the value of main_rc in a windowing environment, to
decide how to display the text. */

/* If in a non-windowing screen run, position to the bottom of the screen
for disastrous errors - this has to be a system-specific thing as we
may be in the middle of screen handling when the error occurs. */

#ifdef Windowing
if (rc >= rc_failed && main_screenmode && !main_windowing) sys_crashposition();
#else
if (rc >= rc_failed && main_screenmode) sys_crashposition();
#endif

/* If we have a disastrous error, turn on logging to the crash log file. This
will cause all subsequent output via error_printf to be copied to this file. 
Don't do this for error 0, however (failure to decode command line), or normal
errors when not initialized (i.e. errors in the init line). */

if (n != 0 && (orig_rc > rc_serious || main_initialized) && rc >= rc_failed) 
  main_logging = TRUE;

/* First, show the command line if relevant */

if (error_data[n].showcmd && (!main_initialized || !main_interactive || main_screenmode))
  {
  int i = cmd_ptr - cmd_cmdline;
  error_printf("%s", cmd_cmdline);
  if (cmd_cmdline[strlen(cmd_cmdline) - 1] != '\n') error_printf("\n");
  if (i)
    {
    while (i--) error_printf(" ");
    error_printf(">\n");
    }
  }

/* Now output the error message */

error_printf("%s", buff);

/* For error 38 - compiling a regular expression - add explanation about the
different types, in case the user is confused. */

if (n == 38)
  {
  error_printf("** The string was interpreted as %s style regular expression.\n",
    main_unixregexp? "a Unix" : "an NE"); 
  } 

/* In a non-windowing environment, E exits on a disaster. In a windowing
environment, it stays alive if there is a way out via longjmp. This may
leave the screen in a bad state, but it does make debugging easier. */

if (rc >= rc_failed)
  {
  if (main_logging)
    error_printf("** Error information is being written to the crash log\n");  
  #ifdef Windowing 
  if (main_windowing && error_longjmpOK)
    {
    error_printf("\n*** Screen display may be incorrect - proceed with caution ***\n");
    longjmp(error_jmpbuf, 1);
    }
  #endif   
  crash_handler(-rc);   /* -n is a pseudo signal value */
  }
}


/*************************************************
*               QS/SE formatter                  *
*************************************************/

/* Sub-function to format flags */

static int flagbits[] = {
  0x0003, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020,
  0x0040, 0x0080, 0x0100, 0x0200, 0x0400, 0};

static char *flagchars = "pbehlnrsuvwx";

static char *format_qseflags(char *b, int flags)
{
int i;
for (i = 0; flagbits[i]; i++)
  {
  if ((flags & flagbits[i]) == flagbits[i])
    {
    *b++ = flagchars[i];
    flags &= ~flagbits[i];
    }
  }
return b;
}

/* Function for formatting a qualified string or search expression.
Avoid the use of ANSI sprintf, to aid portability. Pity... */

static char *format_qse(char *b, sestr *se)
{
int n;

if (se->type == cb_setype)
  {
  if (se->count != 1)
    {
    #ifdef NO_PERCENT_N
    sprintf(b, "%d", se->count);
    n = strlen(b);
    #else
    sprintf(b, "%d%n", se->count, &n);
    #endif
    b += n;
    }
  *b++ = '(';
  b = format_qse(b, se->left.se);
  if (se->right.se != NULL)
    {  
    sprintf(b, " %s ", ((se->flags & qsef_AND) != 0)? "&" : "|");
    b += 3;
    b = format_qse(b, se->right.se);
    } 
  *b++ = ')';
  }

else
  {
  qsstr *qs = (qsstr *)se;
  if (qs->count != 1)
    {
    #ifdef NO_PERCENT_N
    sprintf(b, "%d", qs->count);
    n = strlen(b);
    #else
    sprintf(b, "%d%n", qs->count, &n);
    #endif
    b += n;
    }
  if (qs->windowleft != qse_defaultwindowleft || qs->windowright != qse_defaultwindowright)
    {
    #ifdef NO_PERCENT_N
    sprintf(b, "[%d,%d]", qs->windowleft+1, qs->windowright);
    n = strlen(b);
    #else
    sprintf(b, "[%d,%d]%n", qs->windowleft+1, qs->windowright, &n);
    #endif
    b += n;
    }
  b = format_qseflags(b, qs->flags);
  strncpy(b, qs->text, qs->length + 1);   /* extra 1 to include delimiter */
  b += qs->length + 1;
  *b++ = qs->text[0];
 }

*b = 0;
return b;
}


/*************************************************
*       Wrapper for unpicking se/qs args         *
*************************************************/

/* This function is used in a few cases when a qualified string or search
expression needs to be expanded for inclusion in an error message. So far
there is no requirement for other arguments. */

void error_moanqse(int n, sestr *p)
{
char sebuff[256];
(void)format_qse(sebuff, p);
error_moan(n, sebuff);
}

/* End of eerror.c */
