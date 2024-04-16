/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1993, 1994 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: February 1994 */


/* This file contains debugging code. */

#include "ehdr.h"


/*************************************************
*           Display debugging output             *
*************************************************/

/* Made global so it can be called from debugging code elsewhere */

void debug_printf(char *format, ...)
{
char buff[256];
va_list ap;
va_start(ap, format);
vsprintf(buff, format, ap);
if (main_screenmode)
  {
  if (debug_file == NULL)
    {
    debug_file = fopen("NEdebug", "w");
    if (debug_file == NULL)
      {
      printf("\n\n***** Can't open debug file - aborting *****\n\n");
      exit(99);
      }
    }
  fprintf(debug_file, "%s", buff);
  fflush(debug_file);
  }
else
  {
  printf("%s", buff);
  fflush(stdout);
  }
va_end(ap);
}


/*************************************************
*        Give information about the screen       *
*************************************************/

void debug_screen(void)
{
debug_printf("main_linecount         = %d\n", main_linecount);
debug_printf("main_currentlinenumber = %d\n\n", main_currentlinenumber);
debug_printf("cursor_offset      = %2d\n", cursor_offset);
debug_printf("cursor_row         = %2d cursor_col         = %2d\n",
  cursor_row, cursor_col);
debug_printf("window_width       = %2d window_depth       = %2d\n",
  window_width, window_depth);
debug_printf("-------------------------------------------------\n");
}


/*************************************************
*            Write to a crash log file           *
*************************************************/

void debug_writelog(char *format, ...)
{
va_list ap;
va_start(ap, format);

if (crash_logfile == NULL)
  {
  char *name = sys_crashfilename(FALSE);
  if (name == NULL) return; 
  crash_logfile = fopen(name, "w");
  if (crash_logfile == NULL)
    {
    main_logging = FALSE;    /* Prevent recursion */ 
    error_printf("Failed to open crash log file %s\n", name);
    return;
    }
  }

vfprintf(crash_logfile, format, ap);
fflush(crash_logfile);
va_end(ap);
}

/* End of debug.c */
