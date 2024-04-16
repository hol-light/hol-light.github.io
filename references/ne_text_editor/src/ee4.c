/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1993, 1994 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: November 1994 */


/* This file contains code for obeying commands: Part IV */


#include "ehdr.h"
#include "cmdhdr.h"
#include "keyhdr.h"




/*************************************************
*              The SAVE command                  *
*************************************************/

/* The main procedure is also used by the WRITE command.
For SAVE, if successful, the buffer's name is changed if a
new name was given and the output file is not completely
released. For WRITE, the file is unrelated to the buffer;
the name is not changed, and the file is closed. SAVE is
permitted for stream buffers, but it closes the file and
deletes the buffer, and no file name can be given. */


static int savew(cmdstr *cmd, BOOL saveflag, linestr *line, linestr *last)
{
int type = -1;
int yield = done_continue;
FILE *fid = currentbuffer->to_fid;
BOOL changename = saveflag && (cmd->misc != save_keepname);
char *alias, *name, *savealias;

/* Handle stream buffers */

if (fid != NULL && saveflag && (cmd->flags & cmdf_arg1) != 0)
  {
  error_moan(54, "saving to another file");
  return done_error;
  }

/* Now do the writing */

if ((cmd->flags & cmdf_arg1) == 0)        /* no string */
  {
  type = cmd_confirmoutput(main_filealias, FALSE, FALSE, TRUE, -1, &alias);
  if (type == 0)
    {
    name = main_filename;
    alias = main_filealias;
    changename = FALSE;
    }
  else if (type == 4) name = alias; 
    else { main_repaint = TRUE; return done_error; }
  }

else alias = name = (cmd->arg1.string)->text;     /* string supplied */

/* Ensure the whole file is in store if writing to end; if this is a
stream file, earlier lines must get written out en route. */

if (last == NULL) while (file_extend() != NULL)
  {
  if (fid != NULL) main_checkstream();
  }

/* Open the output appropriately */

if (fid == NULL)
  {
  if (name == NULL || name[0] == 0)
    {
    error_moan(59, currentbuffer->bufferno);
    return done_error;
    }
  fid = sys_fopen(name, "w", NULL);
  }

if (main_screenmode
    #ifdef Windowing
      && !main_windowing
    #endif
   ) error_printf("Writing %s\n", alias);

if (fid == NULL)
  {
  error_moan(5, name);
  return done_error;
  }

/* If new name, reset the buffer's files (cf NAME) */

if (changename)
  {
  if (from_fid != NULL)
    {
    if ((main_bottom->flags & lf_eof) != 0)
      {
      fclose(from_fid);
      from_fid = currentbuffer->from_fid = NULL;
      }
    else currentbuffer->cleof = TRUE;
    }

  store_free(main_filealias);
  store_free(main_filename);
  main_filealias = store_copystring(alias);
  main_filename = store_copystring(alias);
  currentbuffer->filename = main_filename;
  currentbuffer->filealias = main_filealias;

  #ifdef Windowing
  if (main_windowing) sys_window_showmark(mark_type, mark_hold);  /* For new name */
  #endif

  main_drawgraticules |= dg_bottom;
  }

/* Now write the file */

savealias = main_filealias;
main_filealias = alias;       /* for messages from sys_outputline */

while ((line->flags & lf_eof) == 0)
  {
  int rc = file_writeline(line, fid);
  if (rc < 0)
    {
    error_moan(37, alias, strerror(errno));
    return done_error;
    }
  else if (rc == 0) yield = done_error;   /* failed binary */
  if (line == last) break;
  line = line->next;
  }

main_filealias = savealias;   /* restore real name */

fclose(fid);
sys_setfiletype(name, currentbuffer->filetype);

if (saveflag)
  {
  if (yield == done_continue)   /* If no binary errors */
    {
    main_filechanged = FALSE;
    currentbuffer->changed = FALSE;
    currentbuffer->saved = TRUE;
    }

  /* Stream file */

  if (currentbuffer->to_fid != NULL)
    {
    currentbuffer->to_fid = NULL;
    e_dbuffer(cmd);                 /* Guaranteed no data */
    }
  }

if (yield == done_continue) main_nowait = TRUE;
return yield;
}

int e_save(cmdstr *cmd) { return savew(cmd, TRUE, main_top, NULL); }


/*************************************************
*              The SET command                   *
*************************************************/

int e_set(cmdstr *cmd)
{
switch (cmd->misc)
  {
  case set_autovscroll:
  main_vcursorscroll = cmd->arg1.value;
  break;

  case set_splitscrollrow:
  main_ilinevalue = cmd->arg1.value;
  break;

  case set_oldcommentstyle:
  main_oldcomment = TRUE;
  break;

  case set_newcommentstyle:
  main_oldcomment = FALSE;
  break;
  }
return done_continue;
}




/*************************************************
*            The SHOW command                    *
*************************************************/

/* Key showing subroutine used by e_SHOW */

static void showkeysub(int type)
{
char *keychars = "abcdefghijklmnopqrstuvwxyz[\\]^_";
int i;
int offset = 0;
int end = 31;
int f = 0;
int spextra = 0;
int flip = FALSE;

switch(type)
  {
  case show_ckeys:
  error_printf("\nCTRL KEYS\n");
  break;

  case show_fkeys:
  error_printf("\nFUNCTION KEYS\n");
  end = max_fkey;
  offset = s_f_umax;
  f = 1;
  break;

  case show_keystrings:
  error_printf("\nFUNCTION KEYSTRINGS\n");
  end = max_keystring;
  f = 2;
  spextra = 3;
  break;

  case show_actions:
  error_printf("\nKEY ACTIONS\n");
  end = key_actnamecount;
  f = 4;
  break;

  default:
  error_printf("\nEXTRA KEYS\n");
  offset = s_f_ubase - 1;
  end = s_f_umax - s_f_ubase + 1;
  f = 3;
  }

for (i = 1; i <= end; i++)
  {
  int ba = TRUE;
  int spused = spextra;
  int action = (f==4)? key_actnames[i-1].code : (f==2)? i : key_table[i+offset];
  if (action)
    {
    char *s;
    if (1 <= action && action <= max_keystring)
      {
      s = main_keystrings[action];
      ba = FALSE;
      }
    else s = key_actionnames[action - max_keystring - 1];

    if (s)
      {
      int spaces;

      switch (f)
        {
        case 0: error_printf("ctrl/%c ", keychars[i-1]); break;
        case 1: error_printf("fkey %d %s", i, i<10? " ":""); break;
        case 2: error_printf("keystring %d %s",action,action<10? " ":""); break;
        case 3:
          {
          char buff[20];
          key_makespecialname(i+offset, buff);
          error_printf("%s", buff);
          }
        break;
        case 4: error_printf("%-6s ", key_actnames[i-1].name); break;
        }

      if (ba) error_printf("%s", s); else
        {
        if (f==2 || (f==1 && i==action))
          {
          error_printf("\"%s\"", s);
          spused += 2;
          }
        else
          {
          error_printf("%s(%d)\"%s\"", action<10? " ":"", action, s);
          spused += 6;
          }
        }

      spaces = 28 - strlen(s) - spused;
      if (flip || spaces <= 0) { error_printf("\n"); flip = FALSE; } else
        {
        int j;
        for (j = 1; j <= spaces; j++) error_printf(" ");
        flip = TRUE;
        }
      }
    }
  }

if (flip) error_printf("\n");
if (f == 3) sys_specialnotes();  /* System-specific comments */
}


/* Routine proper */

int e_show(cmdstr *cmd)
{
switch (cmd->misc)
  {
  case show_keystrings:
  case show_ckeys:
  case show_fkeys:
  case show_xkeys:
  case show_actions:
  showkeysub(cmd->misc);
  break;

  case show_allkeys:
    {
    static int s[] = { show_ckeys, show_xkeys, show_fkeys };
    int i;
    for (i = 0; i <= 2; i++)
      {
      showkeysub(s[i]);
      #ifdef Windowing
      if (!main_windowing && main_screenOK && i != 2)
      #else
      if (main_screenOK && i != 2)
      #endif
        {
        int c;
        error_printf("Press RETURN to continue ");
        error_printflush();
        while ((c = fgetc(kbd_fid)) != '\n' && c != '\r');
        }
      if (main_interrupted(ci_type)) break;
      }
    }
  break;

  case show_buffers:
    {
    bufferstr *b = main_bufferchain;
    currentbuffer->from_fid = from_fid;         /* Get up to date */
    currentbuffer->changed = main_filechanged;
    currentbuffer->linecount = main_linecount;

    while (b != NULL)
      {
      char *name = b->filealias;
      char *changed = b->changed?  "(modified)" : "          ";
      int c = (b->from_fid == NULL)? ' ' : '+';
      if (name == NULL || name[0] == 0) name = "<unnamed>";
      if (b->to_fid == NULL)
        error_printf("Buffer %d  %5d%c lines %s  %s\n",
          b->bufferno, b->linecount, c, changed, name);
      else error_printf("Buffer %d        stream %s  %s\n",
        b->bufferno, changed, name);
      b = b->next;
      }

    if (cut_buffer != NULL)
      {
      linestr *p = cut_buffer;
      char *pastext = cut_pasted? "(pasted)  " : "          ";
      char *type = (cut_type == cuttype_text)? "<text>" : "<rectangle>";
      char *pad;
      int n = 0;

      while (p != NULL) { n++; p = p->next; }
      pad = (n < 10)? "    " : (n < 100)? "   " : (n < 1000)? "  " :
        (n < 10000)? " " : "";
      error_printf("Cut buffer%s%d  lines %s  %s\n", pad, n, pastext, type);
      }
    }
  break;
  
  case show_wordchars:
    {
    int i;  
    error_printf("Wordchars:"); 
    for (i = 0; i < 256; i++) if ((ch_tab[i] & ch_word) != 0)
      {
      int first = i; 
      error_printf(" %c", i); 
      while (i < 256 && (ch_tab[i+1] & ch_word) != 0) i++;
      if (first != i) error_printf("-%c", i); 
      }  
    error_printf("\n"); 
    }
  break;  

  case show_wordcount:
    {
    int crlfcount = sys_crlfcount();
    LONGINT lc = -1, wc = 0, cc = -crlfcount;   /* allow for eof "line" */
    linestr *line = main_top;
    while (file_extend() != NULL);          /* ensure all in store */
    while (line != NULL)
      {
      int len = line->len;
      char *p = line->text;
      if (main_interrupted(ci_scan)) return done_error;
      cc += len + crlfcount;
      lc++;

      for (;;)
        {
        while (--len >= 0 && p[len] == ' ');
        if (len < 0) break;
        wc++;
        while (--len >= 0 && p[len] != ' ');
        if (len < 0) break;
        }

      line = line->next;
      }

    error_printf("%ld line%s %ld word%s %ld character%s\n",
      lc, (lc==1? "":"s"),
      wc, (wc==1? "":"s"),
      cc, (cc==1? "":"s"));
    }
  break;

  case show_version:
  error_printf("NE version %s %s\n", version_string, version_date);
  break;

  case show_commands:
    {
    int i;
    error_printf("\nCOMMANDS\n");
    for (i = 0; i < cmd_listsize; i++)
      {
      error_printf(" %-14s", cmd_list[i]);
      if (i%5 == 4 || i == cmd_listsize - 1) error_printf("\n");
      }
    }
  break;
  
  case show_settings:
    {
    error_printf("append:         %s\n", main_appendswitch? " on" : "off");
    error_printf("attn:           %s\n", main_attn? " on" : "off");
    if (main_screenmode) 
      error_printf("autoalign:      %s\n", main_AutoAlign? " on" : "off");  
    error_printf("casematch:      %s\n", cmd_casematch? " on" : "off");
    error_printf("detrail output: %s\n", main_detrail_output? " on" : "off");
    error_printf("eightbit:       %s\n", main_eightbit? " on" : "off");
    if (main_screenmode) 
      error_printf("overstrike:     %s\n", main_overstrike? " on" : "off"); 
    error_printf("prompt:         %s\n", currentbuffer->noprompt? "off" : " on");
    error_printf("readonly:       %s\n", main_readonly? " on" : "off"); 
    error_printf("unixregexp:     %s\n", main_unixregexp? " on" : "off");
    if (!main_screenmode) 
      error_printf("verify:         %s\n", main_verify? " on" : "off");  
    error_printf("warn:           %s\n", main_warnings? " on" : "off"); 
    }
  break;      
  }

return done_wait;    /* indicate output produced */
}


/*************************************************
*            The STOP command                    *
*************************************************/

/* If there are modified, unsaved buffers, proceed only if the user
allows it, or if non-interactive. */

int e_stop(cmdstr *cmd)
{

if (main_interactive)
  {
  bufferstr *b = main_bufferchain;
  bufferstr *last = NULL;
  int count = 0;

  currentbuffer->changed = main_filechanged;  /* make up-to-date */

  while (b != NULL)
    {
    if (b != currentbuffer && b->changed)
      {
      count++;
      last = b;
      }
    b = b->next;
    }

  if (count > 0 && main_warnings)
    {
    char buff[100];
    if (count > 1)
      sprintf(buff, "Some buffers have been modified but not saved.\n");
    else if (last->filealias == NULL || (last->filealias)[0] == 0)
      sprintf(buff, "Buffer %d has been modified but not saved.\n", last->bufferno);
    else
      sprintf(buff, "Buffer %d (%s) has been modified but not saved.\n",
        last->bufferno, last->filealias);
    #ifdef Windowing
    if (main_windowing)
      {
      if (!sys_window_yesno(FALSE, "Continue", buff)) return done_error;
      }
    else
    #endif
      {
      error_printf(buff);
      if (!cmd_yesno("Continue with STOP command (Y/N)? ")) return done_error;
      }
    }
  }

main_rc = 8;
return done_finish;
}



/*************************************************
*           The STREAM command                   *
*************************************************/

int e_stream(cmdstr *cmd)
{
int rc;

if (currentbuffer->to_fid != NULL)
  {
  error_moan(54, "a second stream command");
  return done_error;
  }

rc = e_name(cmd);
if (rc != done_continue) return rc;

if (main_stream == 0) main_stream = stream_default;

currentbuffer->to_fid = sys_fopen(main_filealias, "w", NULL);
if (currentbuffer->to_fid == NULL)
  {
  error_moan(5, main_filealias);
  return done_error;
  }
else currentbuffer->streammax = main_stream;

return done_continue;
}



/*************************************************
*            The STREAMMAX command               *
*************************************************/

int e_streammax(cmdstr *cmd)
{
currentbuffer->streammax = main_stream = cmd->arg1.value;
return done_continue;
}



/*************************************************
*              The T and TL commands             *
*************************************************/

int e_ttl(cmdstr *cmd)
{
int i;
int n = cmd->arg1.value;
BOOL flag = cmd->misc;
char *hexlist = "0123456789ABCDEF";
linestr *line = main_current;

if (n < 0) n = BIGNUMBER;    /* -ve comes from *, meaning "to eof" */

for (i = 0; i < n; i++)
  {
  int j;
  int len = line->len;
  char *p = line->text;
  BOOL nonprinters = FALSE;

  if (main_interrupted(ci_type)) return done_error;
  if ((line->flags & lf_eof) != 0) break;

  if (flag)
    {
    if (line->key > 0) error_printf("%4d  ", line->key);
      else error_printf("****  ");
    }

  for (j = 0; j < len; j++)
    {
    int c = p[j];
    if (!isprint(c))
      {
      error_printf("%c", hexlist[(c & 0xf0) >> 4]);
      nonprinters = TRUE;
      }
    else error_printf("%c", c);
    }
  error_printf("\n");

  if (nonprinters)
    {
    if (flag) error_printf("      ");
    for (j = 0; j < len; j++)
      {
      int c = p[j];
      if (!isprint(c)) error_printf("%c", hexlist[c & 0x0f]);
        else error_printf(" ");
      }
    error_printf("\n");
    }

  line = mac_nextline(line);
  }

return done_wait;
}



/*************************************************
*              The TITLE command                 *
*************************************************/

int e_title(cmdstr *cmd)
{
char *s = (cmd->arg1.string)->text;
if (main_filealias != main_filename) store_free(main_filealias);
currentbuffer->filealias = main_filealias = store_copystring(s);
main_drawgraticules |= dg_bottom;
return done_continue;
}



/*************************************************
*            The TOPLINE command                 *
*************************************************/

int e_topline(cmdstr *cmd)
{
#ifdef Windowing
if (main_windowing) sys_window_topline(main_currentlinenumber); else
#endif
scrn_hint(sh_topline, 0, main_current);
return done_continue;
}


/*************************************************
*           The UNDELETE command                 *
*************************************************/

int e_undelete(cmdstr *cmd)
{
if (main_undelete != NULL)
  {
  /* Handle single character undelete */
  if ((main_undelete->flags & lf_udch) != 0)
    {
    char ch = main_undelete->text[--(main_undelete->len)];
    BOOL forwards = main_undelete->text[--(main_undelete->len)];
    if (main_undelete->len <= 0)
      {
      linestr *next = main_undelete->next;
      store_free(main_undelete->text);
      store_free(main_undelete);
      main_undeletecount--;
      main_undelete = next;
      if (next != NULL) next->prev = NULL;
        else main_lastundelete = NULL;
      }

    line_insertch(main_current, cursor_col, &ch, 1, 0);
    main_current->flags |= lf_shn;
    if (!forwards) cursor_col++;
    }

  /* Handle line undelete */
  else
    {
    linestr *new = main_undelete;
    linestr *prev = main_current->prev;

    main_undelete = new->next;
    if (main_undelete == NULL) main_lastundelete = NULL;
      else main_undelete->prev = NULL;
    main_undeletecount--;

    if (prev == NULL) main_top = new; else prev->next = new;
    new->prev = prev;
    new->next = main_current;
    main_current->prev = new;
    main_current = new;          /* Put cursor back on inserted line */
    cursor_col = 0;
    main_linecount++;
    if (main_screenOK) scrn_hint(sh_insert, 1, NULL);
    cmd_refresh = TRUE;
    }

  cmd_recordchanged(main_current, cursor_col);
  cmd_saveback();
  }

return done_continue;
}


/*************************************************
*           The UNIXREGEXP command               *
*************************************************/

int e_unixregexp(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) main_unixregexp = cmd->arg1.value;
  else main_unixregexp = !main_unixregexp;
return done_continue;
}





/*************************************************
*             The VERIFY command                 *
*************************************************/

int e_verify(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) main_verify = cmd->arg1.value;
  else main_verify = !main_verify;

#ifdef Windowing
if (main_verify && !main_shownlogo && !main_windowing)
#else
if (main_verify && !main_shownlogo)
#endif
  {
  error_printf("NE version %s %s\n", version_string, version_date);
  main_shownlogo = TRUE;
  }

return done_continue;
}



/*************************************************
*            The W (windup) command              *
*************************************************/

int e_w(cmdstr *cmd)
{
int count = 0;
bufferstr *thisbuffer = currentbuffer;

/* Warn if data in the cut buffer has not been pasted; prompt
for permission to continue (if non-interactive, answer will
always be 'yes'). */

if (!cut_pasted && cut_buffer != 0 &&
  (cut_buffer->len != 0 || cut_buffer->next != NULL) && main_warnings)
  {
  #ifdef Windowing
  if (main_windowing)
    {
    if (!sys_window_yesno(FALSE, "Continue",
      "The contents of the cut buffer have not been pasted.\n")) return done_error;
    }
  else
  #endif
    {
    if (!cut_overwrite("Continue with W command (Y/N)? ")) return done_error;
    }  
  }


/* Cycle through all the buffers. Since some may get deleted on the way, first
find out how many there are. */

do
  {
  count++;
  thisbuffer = (thisbuffer->next == NULL)?
    main_bufferchain : thisbuffer->next;
  }
while (currentbuffer != thisbuffer);

while (count-- > 0)
  {
  BOOL writeneeded = FALSE;
  bufferstr *nextbuffer = (currentbuffer->next == NULL)?
    main_bufferchain : currentbuffer->next;
  int n = currentbuffer->bufferno;
  char *newname = NULL;

  if (main_filechanged)
    {
    int x = cmd_confirmoutput(main_filealias, TRUE, TRUE,
      (currentbuffer->to_fid == NULL),
      ((currentbuffer == nextbuffer && n == 0)? -1 : n), &newname);

    switch (x)
      {
      case 0:                /* Yes */
      writeneeded = TRUE;
      break;

      case 1:
      if (main_screenOK) screen_forcecls = TRUE;
      return done_error;     /* No */

      case 2:
      return e_stop(cmd);    /* Stop */

      case 3:                /* Discard */
      break;

      case 4:                /* New file name */
      writeneeded = TRUE;
      main_drawgraticules |= dg_bottom;
      break;
      }
    }

  else if (currentbuffer == thisbuffer)
    {
    if (main_filealias == NULL)
      error_printf("No changes made to buffer %d", n);
        else error_printf("No changes made to %s", main_filealias);

    if (currentbuffer->saved)
      error_printf(" since last SAVE\n");
        else error_printf("\n");
    }

  if (writeneeded)
    {
    if (newname == NULL) newname = main_filename;

    #ifdef Windowing
    if (main_screenOK && !main_windowing)
    #else
    if (main_screenOK)
    #endif

    { sys_mprintf(msgs_fid, "\r"); }

    /* A stream buffer may be partly written, and we don't want to
    have it all in store at once, so it has a separate writing
    routine. Also, its buffer must be deleted afterwards. The name
    is handed over only for use in error messages. */

    if (currentbuffer->to_fid != NULL)
      {
      if (!file_streamout(newname)) return done_error;
      main_filechanged = FALSE;
      currentbuffer->saved = TRUE;
      setup_dbuffer(currentbuffer);
      }

    /* A non-stream buffer is handled separately, as its output
    file may be the same as its input file. If writing fails in
    an interactive run, return immediately with an error. Otherwise
    carry on, in the hope of writing the other buffers. An error
    message will have been given, so the return code should get
    set. */

    else
      {
      if (file_save(newname)) 
        {
        main_filechanged = FALSE;
        currentbuffer->saved = TRUE;
        }
      else if (main_interactive) return done_error;
      }
    }

  init_selectbuffer(nextbuffer, FALSE);
  }

return done_finish;
}


/*************************************************
*            The WARN command                    *
*************************************************/

int e_warn(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) main_warnings = cmd->arg1.value;
  else main_warnings = !main_warnings;
return done_continue;
}


/*************************************************
*                The WHILE command               *
*************************************************/

/* This procedure is also called by UNTIL -- a switch
in the command block distinguishes. The absence of
a search expression means "test for eof". */

int e_while(cmdstr *cmd)
{
int yield = done_loop;
int misc = cmd->misc;
BOOL oldeoftrap = cmd_eoftrap;
BOOL prompt = (misc & if_prompt) != 0;
sestr *se = ((cmd->flags & cmdf_arg1) == 0)? NULL : cmd->arg1.se;

cmd_eoftrap = (se == NULL && ((misc & (if_mark | if_eol | if_sol | if_sof)) == 0));

while (yield == done_loop)
  {
  yield = done_continue;
  while (yield == done_continue)
    {
    BOOL match;
    if (main_interrupted(ci_loop)) return done_error;

    if (prompt)
      match = cmd_yesno(cmd->arg1.string->text);
    else if (se == NULL)
      {
      if ((misc & if_mark) != 0)
        match = (mark_type == mark_lines) && (mark_line == main_current);
      else if ((misc & if_eol) != 0)
        match = (cursor_col >= main_current->len);
      else if ((misc & if_sol) != 0)
        match = (cursor_col == 0);
      else if ((misc & if_sof) != 0)
        match = (cursor_col == 0 && main_current->prev == NULL);
      else match = (main_current->flags & lf_eof) != 0;
      }
    else
      {
      int USW = 0;
      if (!cmd_casematch) USW |= qsef_U;
      match_L = FALSE;
      match_leftpos = cursor_col;
      match_rightpos = main_current->len;
      match = cmd_matchse(se, main_current, USW);
      }
    if (misc >= if_unless) match = !match;

    if (match) yield = cmd_obeyline(cmd->arg2.cmds); else break;
    }

  if (yield == done_loop || yield == done_break)
    {
    if (--cmd_breakloopcount > 0) break;
    if (yield == done_break) yield = done_continue;
    }
  }

if (yield == done_eof && !oldeoftrap) yield = done_continue;
cmd_eoftrap = oldeoftrap;
return yield;
}


/*************************************************
*            The WORD command                    *
*************************************************/

int e_word(cmdstr *cmd)
{
int i;
char *p = cmd->arg1.string->text;

for (i = 0; i < 256; i++) ch_tab[i] &= ~ch_word;

while (*p)
  {
  int a = *p++;
  if (a == '\"') a = *p++;
  ch_tab[a] |= ch_word;
  if (*p == '-')
    {
    int b = *(++p);
    p++;
    for (i = a+1; i <= b; i++) ch_tab[i] |= ch_word;
    }
  }
return done_continue;
}



/*************************************************
*            The WRITE command                   *
*************************************************/

/* Most of the code is shared with SAVE */

int e_write(cmdstr *cmd)
{
linestr *first = main_top, *last = NULL;

if (mark_type == mark_lines)
  {
  if (line_checkabove(mark_line) > 0)
    { first = mark_line; last = main_current; }
  else
    { first = main_current; last = mark_line; }

  if (!mark_hold)
    {
    if (mark_line != NULL) mark_line->flags |= lf_shn;
    mark_type = mark_unset;
    mark_line = NULL;
    }
  }

return savew(cmd, FALSE, first, last);
}



/*************************************************
*           System Call Command                  *
*************************************************/

int e_star(cmdstr *cmd)
{
char *s = NULL;

#ifdef Windowing
if (main_windowing)
  {
  error_moan(15, "*commands", "in windowing mode");
  return done_continue;
  }
#endif

if ((cmd->flags & cmdf_arg1) != 0) s = (cmd->arg1.string)->text;

if (main_screenOK && screen_suspend)
  {
  printf("\r\n");
  scrn_suspend();
  }
   
if (s != NULL) system(s);

return done_wait;
}

/* End of ee4.c */

