/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1997 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: December 1997 */


/* This file contains code for handling an E crash. */

#include "ehdr.h"

static BOOL AllowDump = TRUE;



/*************************************************
*       Open the crash recovery file             *
*************************************************/

static FILE *open_crashfile(char *name)
{
FILE *fid = NULL;
if (crash_handler_chatty)
  error_printf("\n** Attempting to write data to %s\n", name);
fid = sys_fopen(name, "w", NULL);
if (fid == NULL && crash_handler_chatty)
  {
  error_printf("** Failed to open %s\n", name);
  }
return fid;
}



/*************************************************
*              Dump buffers after crash          *
*************************************************/

static void dump_buffers(char *name)
{
FILE *fid = NULL;
bufferstr *firstbuffer = currentbuffer;
linestr *line;
int count;

/* Write the cut buffer if not pasted */

if (!cut_pasted)
  {
  if ((fid = open_crashfile(name)) == NULL) return;
  fprintf(fid, ">>>>> Cut Buffer >>>>>\n");
  line = cut_buffer;
  count = 0;
  while (line != NULL)
    {
    if (file_writeline(line, fid) < 0 && crash_handler_chatty) 
      error_moan(37, name, strerror(errno));
    line = line->next;
    count++;
    }
  fprintf(fid, "\n");
  if (crash_handler_chatty) 
    error_printf("** %d line%s written from the cut buffer\n", count,
      (count == 1)? "" : "s");
  }

/* Repeat for all buffers that have changed. */

do
  {
  bufferstr *nextbuffer = (currentbuffer->next == NULL)?
    main_bufferchain : currentbuffer->next;

  count = 0;

  if (main_filechanged)
    {
    if (fid == NULL && (fid = open_crashfile(name)) == NULL) return;
    fprintf(fid, ">>>>> Buffer %d", currentbuffer->bufferno);
    if (currentbuffer->filealias != NULL)
      fprintf(fid, " (%s)", currentbuffer->filealias);
    fprintf(fid, " >>>>>\n");
    line = main_top;

    /* Write the lines currently in store */

    while (line->next != NULL)
      {
      if (file_writeline(line, fid) < 0 && crash_handler_chatty) 
        error_moan(37, name, strerror(errno));
      line = line->next;
      count++;
      }

    /* If not at end of file, read and write the remaining lines,
    starting with the current last one. As we don't want to fill
    up oodles of store, free each one as we go along. Don't pass
    the buffer to nextline, to avoid any checks on stream buffers. */

    while (line != NULL && (line->flags & lf_eof) == 0)
      {
      file_writeline(line, fid);
      count++;
      store_free(line->text);
      store_free(line);
      if (from_fid == NULL) break;
      line = file_nextline(&from_fid, &currentbuffer->binoffset, NULL);
      }

    fprintf(fid, "\n");

    /* Verify what's been done */

    if (crash_handler_chatty)
      { 
      error_printf("** %d line%s written", count, (count == 1)? "" : "s");
      error_printf(" from buffer %d", currentbuffer->bufferno);
      if (currentbuffer->filealias != NULL)
        error_printf(" (%s)", currentbuffer->filealias);
      }   
    }

  else if (crash_handler_chatty)
    {
    error_printf("** No changes made to buffer %d",
      currentbuffer->bufferno);
    if (currentbuffer->filealias != NULL)
      error_printf(" (%s)", currentbuffer->filealias);
    if (currentbuffer->saved) error_printf(" since last SAVE");
    }

  if (crash_handler_chatty) error_printf("\n");
  if (from_fid != NULL) fclose(from_fid);
  if (currentbuffer->to_fid != NULL) fclose(currentbuffer->to_fid);

  init_selectbuffer(nextbuffer, FALSE);
  }
while (currentbuffer != firstbuffer);  /* end of do loop */

fclose(fid);
}


/*************************************************
*          Signal handler for disasters          *
*************************************************/

/* This function is entered directly from signal handlers, in which
case it is given a positive signal argument. It is also called from
error_moan after a disastrous error, with a negative argument. In the
former case, it itself calls error_moan, in order to put out the message
and get the terminal into the right state. Then it will be called
recursively. However, it won't return from that call. If this is called
with chatty == FALSE, it doesn't try to output anything. This is used
for SIGHUP handling. */

void crash_handler(int sig)
{
if (crash_handler_chatty)
  {
  int i; 
  if (sig > 0)
    {
    for (i = 0; signal_list[i] >= 0; i++) if (signal_list[i] == sig) break;
    error_moan(36, sig, signal_names[i]);
    }
  
  error_printf("** NE Abandoned\n");
  
  /* Show the previous command lines to help with debugging */
  
  if (cmd_stackptr > 0)
    {
    error_printf("\nPrevious command lines:\n");
    for (i = 0; i < cmd_stackptr; i++)
      {
      char *s = cmd_stack[i];
      char *n = (s[strlen(s)-1] == '\n')? "" : "\n";
      error_printf("%s%s", s, n);
      }
    }
  }   

/* If there are any modified buffers, try to dump them out. Note that if
there is an error while writing the file, this function will be called
again. */

if (main_initialized && AllowDump)
  {
  /* There is something really weird going on, at least on Solaris 2 using
  gcc. With SIGHUP, unless there is some kind of function call here, it
  never obeys this slice of code. I haven't been able to track down why
  this is, but the following line of code gets round it. On some other OS,
  SIGHUP just never seems to get here at all... */  
  
  if (sig == SIGHUP) signal(SIGHUP, SIG_DFL);
 
  AllowDump = FALSE;          /* Prevent recursion */
  main_initialized = FALSE;   /* Any more errors are fatal */
  dump_buffers(sys_crashfilename(TRUE));
  }

/* That's all folks! */

if (crash_logfile != NULL) fclose(crash_logfile);
if (debug_file != NULL) fclose(debug_file);

exit(sys_rc(24));
}

/* End of ecrash.c */
