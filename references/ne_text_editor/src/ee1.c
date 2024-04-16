/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1999 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: July 1999 */


/* This file contains code for obeying commands: Part I */


#include "ehdr.h"
#include "cmdhdr.h"



/*************************************************
*         Single-character commands              *
*************************************************/

int e_right(cmdstr *cmd)
{
if ((main_current->flags & lf_eof) != 0)
  {
  if (main_eoftrap) return done_eof;
  error_moan(30, "end of file", ">");
  return done_error;
  }
cursor_col += cmd->count;
return done_continue;
}

int e_left(cmdstr *cmd)
{
if ((main_current->flags & lf_eof) != 0)
  {
  if (main_eoftrap) return done_eof;
  error_moan(30, "end of file", "<");
  return done_error;
  }
cursor_col -= cmd->count;
if (cursor_col < 0) cursor_col = 0;
return done_continue;
}

int e_query(cmdstr *cmd)
{
line_verify(main_current, TRUE, !main_screenOK);
return done_wait;
}

int e_sharp(cmdstr *cmd)
{
if ((main_current->flags & lf_eof) != 0)
  {
  if (main_eoftrap) return done_eof;
  error_moan(30, "end of file", "#");
  return done_error;
  }
line_deletech(main_current, cursor_col, cmd->count, TRUE);
main_current->flags |= lf_shn;
return done_continue;
}

static int e_dolcent(cmdstr *cmd, int (*func)(int), char *name)
{
char *p = main_current->text;
int i;
int len = main_current->len;

if ((main_current->flags & lf_eof) != 0)
  {
  if (main_eoftrap) return done_eof;
  error_moan(30, "end of file", name);
  return done_error;
  }

for (i = 0; i < cmd->count; i++)
  {
  if (cursor_col < len) p[cursor_col] = func(p[cursor_col]);
  cursor_col++;
  }

main_current->flags |= lf_shn;
cmd_recordchanged(main_current, cursor_col);
return done_continue;
}

int e_dollar(cmdstr *cmd)
{
return e_dolcent(cmd, tolower, "$");
}

int e_percent(cmdstr *cmd)
{
return e_dolcent(cmd, toupper, "%");
}


int e_tilde(cmdstr *cmd)
{
char *p = main_current->text;
int i;
int len = main_current->len;

if ((main_current->flags & lf_eof) != 0)
  {
  if (main_eoftrap) return done_eof;
  error_moan(30, "end of file", "~");
  return done_error;
  }

for (i = 0; i < cmd->count; i++)
  {
  if (cursor_col < len)
    {
    int c = p[cursor_col];
    p[cursor_col] = isupper(c)? tolower(c) : toupper(c);
    }
  cursor_col++;
  }

main_current->flags |= lf_shn;
cmd_recordchanged(main_current, cursor_col);
return done_continue;
}




/*************************************************
*            Obey Procedure                      *
*************************************************/

int e_obeyproc(cmdstr *cmd)
{
procstr *p;

if (cmd_findproc(cmd->arg1.string->text, &p))
  {
  int yield;
  BOOL wasactive = (p->flags & pr_active) != 0;
  p->flags |= pr_active;
  yield = cmd_obeyline(p->body);
  if (!wasactive) p->flags &= ~pr_active;
  return yield;
  }
else
  {
  error_moan(48, cmd->arg1.string->text);
  return done_error;
  }
}


/*************************************************
*    Align and other operations on line group    *
*************************************************/

int e_actongroup(cmdstr *cmd)
{
int misc = cmd->misc;
int oneline = (mark_type != mark_lines);

linestr *line = oneline? main_current : mark_line;
linestr *endline = main_current;

static char *cname[] = {
  "align", "dline", "dright", "dleft", "closeup" };

if (!oneline && line_checkabove(mark_line) < 0)
  {
  line = main_current;
  endline = mark_line;
  }

if (!oneline && (!mark_hold || misc == lb_delete))
  { mark_type = mark_unset; mark_line = NULL; }

/* If operating on one line only, and it is the eof line, moan or trap */

if ((line->flags & lf_eof) != 0)
  {
  if (cmd_eoftrap) return done_eof;
  error_moan(30, "end of file", cname[misc]);
  return done_error;
  }

/* Deal with deletion separately */

if (misc == lb_delete)
  {
  for (;;)
    {
    if ((line->flags & lf_eof) != 0) break; else
      {
      int done = (line == endline);
      line = line_delete(line, TRUE);
      if (done) break;
      }
    }
  cmd_refresh = TRUE;
  }

/* Deal with all other actions */

else for (;;)
  {
  int action;

  if ((line->flags & lf_eof) == 0) switch (misc)
    {
    case lb_align:
    line_leftalign(line, cursor_col, &action);
    break;

    case lb_eraseright:
    if (cursor_col < line->len)
      line_deletech(line, cursor_col, line->len - cursor_col, TRUE);
    break;

    case lb_eraseleft:
    line_deletech(line, cursor_col, cursor_col, FALSE);
    break;

    case lb_closeup:
      {
      int i;
      int count = 0;
      char *p = line->text;
      for (i = cursor_col; i < line->len; i++)
        if (p[i] == ' ') count++; else break;
      line_deletech(line, cursor_col, count, TRUE);
      }
    break;

    case lb_closeback:
      {
      int i;
      int count = 0;
      char *p = line->text;
      for (i = cursor_col-1; i >= 0; i--)
        if (p[i] == ' ') count++; else break;
      line_deletech(line, cursor_col-count, count, TRUE);
      cursor_col -= count;
      }
    break;
    }

  line->flags |= lf_shn;
  if (line == endline) break;
  line = line->next;
  }

main_current = line;
if (misc == lb_eraseleft || misc == lb_delete) cursor_col = 0;
return done_continue;
}



/*************************************************
*           The A, B & E commands                *
*************************************************/

 /* They all call the same routine */

int e_abe(cmdstr *cmd)
{
int yield = done_continue;
int oldrmargin = main_rmargin;
int USW = 0;
int misc = cmd->misc;
BOOL stringsearch, REreplace;

sestr *se = cmd->arg1.se;
qsstr *nt = cmd->arg2.qs;

/* Complain if at eof, unless in until eof loop */

if ((main_current->flags & lf_eof))
  {
  if (cmd_eoftrap) return done_eof;
  error_moan(30, "End of file", "a, b, or e");
  return done_error;
  }

/* Deal with saving arguments or re-using old ones */

if ((cmd->flags & cmdf_arg1) != 0)
  {
  if (last_abese != NULL) cmd_freeblock((cmdblock *)last_abese);
  if (last_abent != NULL) cmd_freeblock((cmdblock *)last_abent);
  last_abese = cmd_copyblock((cmdblock *)se);
  last_abent = cmd_copyblock((cmdblock *)nt);
  }
else if (last_abese == NULL)
  {
  error_moan(16, "a, b, or e command");
  return done_error;
  }
else
  {
  se = last_abese;
  nt = last_abent;
  }

/* Final preparations */

stringsearch = (se->type == cb_qstype) &&
  ((((qsstr *)se)->flags & qsef_N) == 0);

if (!stringsearch && cursor_col != 0)
  { error_moan(40); return done_error; }

REreplace = (nt->flags & qsef_R) != 0;

/* No Journal for PCRE regex */

if (stringsearch && (((qsstr *)se)->flags & (qsef_R|qsef_RP)) == qsef_R)
  R_Journal = store_Xget(JournalSize);

match_L = FALSE;
if (!cmd_casematch) USW |= qsef_U;
if (main_rmargin < MAX_RMARGIN) main_rmargin += MAX_RMARGIN;

/* Search for given context */

match_leftpos = cursor_col;
match_rightpos = main_current->len;

/* Take action according as matched or not */

if (cmd_matchse(se, main_current, USW))
  {
  char *p = nt->text + 1;
  int len = nt->length;

  if (misc == abe_a) cursor_col = match_end - 1;
    else cursor_col = match_start;

  /* Regular Expression Change: if required wild
  strings are unavailable, give error and exit. */

  if (REreplace)
    {
    int ok;
    main_current = cmd_ReChange(main_current, p, len, (nt->flags & qsef_X) != 0,
      misc == abe_e, misc == abe_a, &ok);
    if (!ok)
      {
      error_moan(39);
      yield = done_error;
      }
    }

  /* Normal Change */
  else
    {
    if ((nt->flags & qsef_X) != 0)
      {
      p = nt->hexed;
      len /= 2;
      }

    if (misc == abe_e)
      {
      line_deletech(main_current, match_start, match_end-match_start, TRUE);
      line_insertch(main_current, match_start, p, len, 0);
      cursor_col = match_start + len;
      }
    else
      {
      line_insertch(main_current, ((misc == abe_a)? match_end : match_start), p, len, 0);
      cursor_col = match_end + len;
      }
    }

  /* Note line has changed */
  main_current->flags |= lf_shn;
  }

/* No match has been found */

else
  {
  error_moanqse(17, se);      /* not found */
  yield = done_error;
  }

/* Restore rmargin before exit */

main_rmargin = oldrmargin;
if (R_Journal != NULL) { store_free(R_Journal); R_Journal = NULL; }
return yield;
}



/*************************************************
*           The ABANDON command                  *
*************************************************/

int e_abandon(cmdstr *cmd)
{
main_rc = 8;
return done_finish;
}



/*************************************************
*              The ATTN command                  *
*************************************************/

int e_attn(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) main_attn = cmd->arg1.value;
  else main_attn = !main_attn;
if (main_attn && main_oneattn)
  { main_oneattn = FALSE; error_moan(23); return done_error; }
else return done_continue;
}



/*************************************************
*          The AUTOALIGN command                 *
*************************************************/

int e_autoalign(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) main_AutoAlign = cmd->arg1.value;
  else main_AutoAlign = !main_AutoAlign;
main_drawgraticules |= dg_flags;
return done_continue;
}



/*************************************************
*             The BACK command                   *
*************************************************/

/* We recompute the current line number explicitly. This doesn't take very
long and it saves a lot of hassle. It also guarantees that we reset it to
the correct value from time to time! */

int e_back(cmdstr *cmd)
{
int prevptr = ((main_backprevptr >= 0)? main_backprevptr : main_backptr) - 1;

cmd_saveback();

if (prevptr < 0)
  {
  prevptr = back_size - 1;
  while (prevptr > 0 && (main_backlist[prevptr]).line == NULL) prevptr--;
  }

if ((main_backlist[prevptr]).line != NULL)
  {
  linestr *line = main_top;
  backstr *b = main_backlist + prevptr;
  int n = 0;

  while (line != b->line)
    {
    if (line == NULL)
      {
      error_moan(61);
      return done_error;
      }
    n++;
    line = line->next;
    }

  main_currentlinenumber = n;
  main_current = b->line;
  cursor_col = b->col;
  main_backprevptr = prevptr;


  #ifdef Windowing
  if (main_windowing) cursor_row = main_currentlinenumber;
  #endif
  }

return done_continue;
}



/*************************************************
*           The BACKUP command                   *
*************************************************/

/* Currently very restricted */

int e_backup(cmdstr *cmd)
{
switch(cmd->misc)
  {
  case backup_files:
  if ((cmd->flags & cmdf_arg1) != 0) main_backupfiles = cmd->arg1.value;
    else main_backupfiles = !main_backupfiles;
  break;
  }
return done_continue;
}



/*************************************************
*            The BEGINPAR command                *
*************************************************/

int e_beginpar(cmdstr *cmd)
{
cmd_freeblock((cmdblock *)par_begin);
par_begin = cmd_copyblock((cmdblock *)cmd->arg1.se);
return done_continue;
}


/*************************************************
*             The BREAK command                  *
*************************************************/

int e_break(cmdstr *cmd)
{
cmd_breakloopcount = ((cmd->flags & cmdf_arg1) != 0)? cmd->arg1.value : 1;
return done_break;
}


/*************************************************
*              The BUFFER command                *
*************************************************/

int e_buffer(cmdstr *cmd)
{
bufferstr *new = main_bufferchain;

if ((cmd->flags & cmdf_arg1) != 0)
  {
  new = cmd_findbuffer(cmd->arg1.value);
  if (new == NULL)
    {
    error_moan(26, cmd->arg1.value);
    return done_error;
    }
  }

/* cycle backwards */

else if (cmd->misc)
  {
  while (new->next != currentbuffer)
    {
    if (new->next == NULL) break;
    new = new->next;
    }
  }

/* cycle forwards */

else if (currentbuffer->next != NULL) new = currentbuffer->next;

if (new != currentbuffer)
  {
  #ifdef Windowing
  if (main_windowing) sys_window_selectbuffer(new); else
  #endif
  init_selectbuffer(new, FALSE);
  }
return done_continue;
}


/*************************************************
*              The C command                     *
*************************************************/

int e_c(cmdstr *cmd)
{
FILE *oldcfile = cmdin_fid;
linestr *oldcbufferline = cmd_cbufferline;
BOOL wasinteractive = main_interactive;
BOOL wasbinary = main_binary;
int oldclineno = cmd_clineno;
int oldblevel = cmd_bracount;
int oldonecommand = cmd_onecommand;
int yield = done_continue;
char *name = (cmd->arg1.string)->text;

/* Turn off binary mode while opening the command file, as
this is necessary for the DOS version of NE. */

main_binary = FALSE;
cmdin_fid = sys_fopen(name, "r", NULL);
main_binary = wasbinary;

if (cmdin_fid == NULL)
  {
  cmdin_fid = oldcfile;
  error_moan(5, name);
  return done_error;
  }

/* Now obey the lines in the file */

cmd_cbufferline = NULL;
cmd_onecommand = main_interactive = FALSE;
cmd_clineno = 0;

while (yield == done_continue)
  {
  int n;
  if (fgets(cmd_buffer, cmd_buffer_size, cmdin_fid) == NULL) break;
  cmd_clineno++;
  n = strlen(cmd_buffer);
  if (n > 0 && cmd_buffer[n-1] == '\n') cmd_buffer[n-1] = 0;
  yield = cmd_obey(cmd_buffer);

  if (yield == done_error)
    error_printf("c command abandoned after obeying line %d of %s\n", cmd_clineno, name);

  else if (yield == done_wait)
    {
    if (main_screenOK)
      {
      scrn_rdline(TRUE, FALSE, "Press RETURN to continue ");
      error_printf("\n");
      }
    yield = done_continue;
    }
  else if (yield == done_break || yield == done_loop)
    yield = done_continue;
  }
fclose(cmdin_fid);
cmd_clineno = oldclineno;
cmd_cbufferline = oldcbufferline;
main_interactive = wasinteractive;
cmdin_fid = oldcfile;
cmd_bracount = oldblevel;
cmd_onecommand = oldonecommand;
return yield;
}


/*************************************************
*           The CASEMATCH command                *
*************************************************/

int e_casematch(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) cmd_casematch = cmd->arg1.value;
  else cmd_casematch = !cmd_casematch;
main_drawgraticules |= dg_flags;
return done_continue;
}



/*************************************************
*           The C(D)BUFFER command               *
*************************************************/

int e_cdbuffer(cmdstr *cmd)
{
linestr *oldcbufferline = cmd_cbufferline;
FILE *oldcfile = cmdin_fid;
BOOL wasinteractive = main_interactive;
int oldclineno = cmd_clineno;
int oldblevel = cmd_bracount;
int oldonecommand = cmd_onecommand;
int yield = done_continue;
int number = cmd->arg1.value;
linestr *line;
bufferstr *buffer = main_bufferchain;

if (main_binary)
  {
  error_moan(61);
  return done_error;
  }

if ((cmd->flags & cmdf_arg1) != 0) while (buffer->bufferno != number)
  {
  buffer = buffer->next;
  if (buffer == NULL)
    {
    error_moan(26, number);
    return done_error;
    }
  }
else buffer = currentbuffer;

/* Obey the commands */

cmdin_fid = NULL;
buffer->commanding++;
cmd_onecommand = main_interactive = FALSE;
cmd_clineno = 0;
line = buffer->top;

while (yield == done_continue)
  {
  if (line == NULL || (line->flags & lf_eof) != 0) break;

  if (line->len > cmd_buffer_size - 1)
    {
    error_moan(56);
    yield = done_error;
    }
  else
    {
    if (line->len > 0) memcpy(cmd_buffer, line->text, line->len);
    cmd_buffer[line->len] = 0;
    cmd_clineno++;
    cmd_cbufferline = line->next;
    yield = cmd_obey(cmd_buffer);
    }

  if (yield == done_error)
    error_printf("** c%sbuffer command abandoned after obeying line %d of buffer %d\n",
      (cmd->misc == cbuffer_cd)? "d":"", cmd_clineno, buffer->bufferno);

  else if (yield == done_wait)
    {
    if (main_screenOK)
      {
      scrn_rdline(TRUE, FALSE, "Press RETURN to continue ");
      error_printf("\n");
      }
    yield = done_continue;
    }
  else if (yield == done_break || yield == done_loop)
    yield = done_continue;

  line = cmd_cbufferline;
  }

main_interactive = wasinteractive;
cmdin_fid = oldcfile;
cmd_clineno = oldclineno;
cmd_cbufferline = oldcbufferline;
cmd_bracount = oldblevel;
cmd_onecommand = oldonecommand;

/* Mark the buffer not changed */

buffer->commanding--;
buffer->changed = FALSE;
if (buffer == currentbuffer) main_filechanged = FALSE;

/* Delete the buffer if so requested, and return */

if (cmd->misc == cbuffer_cd && yield != done_error) return e_dbuffer(cmd);
return yield;
}



/*************************************************
*              The CENTRE command                *
*************************************************/

int e_centre(cmdstr *cmd)
{
if ((main_current->flags & lf_eof) == 0)
  {
  int width = main_rmargin;
  int len = main_current->len;
  char *p = main_current->text;
  int i = 0;
  int leading, count;

  /* Find start of data in line */

  while (i < len && p[i] == ' ') i += 1;

  /* If margin disabled, use the remembered value */

  if (width > MAX_RMARGIN) width -= MAX_RMARGIN;

  /* Compute how many leading spaces we need */

  leading = (width - (len - i))/2;

  /* Add or delete some */

  line_leftalign(main_current, leading, &count);
  if (count != 0) { main_current->flags |= lf_shn; main_filechanged = TRUE; }
  }

return done_continue;
}



/*************************************************
*                 The CL command                 *
*************************************************/

int e_cl(cmdstr *cmd)
{
qsstr *qs = cmd->arg1.qs;
char *s = "";
int slen = 0;
int len = main_current->len;

if ((cmd->flags & cmdf_arg1) != 0)
  {
  if ((qs->flags & qsef_X) == 0)
    {  
    s = qs->text + 1;
    slen = qs->length;
    } 
  else
    {
    s = qs->hexed;
    slen = qs->length / 2;
    }
  }   

if ((main_current->flags & lf_eof) != 0)
  {
  if (cmd_eoftrap) return done_eof;
  error_moan(30, "End of file", "cl");
  return done_error;
  }
else
  {
  char *p;
  int i;
  linestr *nextline = mac_nextline(main_current);

  if ((nextline->flags & lf_eof) != 0)
    {
    if (cmd_eoftrap) return done_eof;
    error_moan(30, "End of file", "cl");
    return done_error;
    }

  if (cursor_col > len)
    {
    line_insertch(main_current, cursor_col, NULL, 0, 0);
    len = main_current->len;
    }

  main_current = line_concat(nextline, slen);
  p = main_current->text;

  for (i = 0; i < slen; i++) p[len++] = s[i];
  cursor_col = len;
  main_current->flags |= lf_shn;
  cmd_refresh = TRUE;
  return done_continue;
  }
}



/*************************************************
*             The COMMENT command                *
*************************************************/

int e_comment(cmdstr *cmd)
{
error_printf("%s\n", cmd->arg1.string->text);
return done_wait;
}



/*************************************************
*              The CPROC command                 *
*************************************************/

int e_cproc(cmdstr *cmd)
{
procstr *p;
if (cmd_findproc(cmd->arg1.string->text, &p))
  {
  if ((p->flags & pr_active) != 0)
    {
    error_moan(47, cmd->arg1.string->text);
    return done_error;
    }
  else  /* A found procedure is always moved to the top of the list */
    {
    main_proclist = p->next;
    cmd_freeblock((cmdblock *)p);
    return done_continue;
    }
  }
else
  {
  error_moan(48, cmd->arg1.string->text);
  return done_error;
  }
}


/*************************************************
*             CUT, COPY & DMARKED                *
*************************************************/

static int ccd(cmdstr *cmd, char *s)
{
if (mark_type != mark_text && mark_type != mark_rect)
  {
  error_moan(41, s);
  return done_error;
  }
else
  {
  linestr *line = mark_line;
  int type = mark_type;

  if (mark_line != NULL) mark_line->flags |= lf_shn;
  mark_type = mark_unset;
  mark_line = NULL;

  if (cut_cut(line, mark_col, type, s[1] == 'o', s[0] == 'd'))
    {
    if (s[1] != 'o') cmd_refresh = TRUE;
    return done_continue;
    }
  else return done_error;
  }
}

int e_cut(cmdstr *cmd)     { return ccd(cmd, "cut"); }
int e_copy(cmdstr *cmd)    { return ccd(cmd, "copy"); }
int e_dmarked(cmdstr *cmd) { return ccd(cmd, "dmarked"); }



/*************************************************
*          The CUTSTYLE command                  *
*************************************************/

int e_cutstyle(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) main_appendswitch = cmd->arg1.value;
  else main_appendswitch = !main_appendswitch;
main_drawgraticules |= dg_flags;
return done_continue;
}



/*************************************************
*            Cursor moving commands              *
*************************************************/

int e_csd(cmdstr *cmd)
{
linestr *next = mac_nextline(main_current);
if (next == NULL)
  {
  error_moan(30, "end of file", "csd");
  return done_error;
  }
else
  {
  main_current = next;
  main_currentlinenumber++;
  cmd_saveback();
  return done_continue;
  }
}

int e_csu(cmdstr *cmd)
{
linestr *prev = main_current->prev;
if (prev == NULL)
  {
  error_moan(30, "start of file", "csu");
  return done_error;
  }
else
  {
  main_current = prev;
  main_currentlinenumber--;
  cmd_saveback();
  return done_continue;
  }
}


/*************************************************
*           The DBUFFER command                  *
*************************************************/

int e_dbuffer(cmdstr *cmd)
{
bufferstr *buffer = main_bufferchain;
int number = cmd->arg1.value;

if ((cmd->flags & cmdf_arg1) != 0) while (buffer->bufferno != number)
  {
  buffer = buffer->next;
  if (buffer == NULL)
    {
    error_moan(26, number);
    return done_error;
    }
  }
else buffer = currentbuffer;

/* If in use as a command buffer, can't delete */

if (buffer->commanding > 0)
  {
  error_moan(50, buffer->bufferno);
  return done_error;
  }

#ifdef Windowing
if (main_windowing) return sys_window_close(buffer); else
#endif
return setup_dbuffer(buffer);
}


/* Actually do the deletion of a given buffer */

int setup_dbuffer(bufferstr *deletebuffer)
{
bufferstr *buffer = main_bufferchain;
bufferstr **link = &main_bufferchain;

while (buffer != deletebuffer)
  {
  link = &buffer->next;
  buffer = buffer->next;
  if (buffer == NULL)
    {
    error_moan(26, deletebuffer->bufferno);
    return done_error;
    }
  }

/* Empty the buffer */

if (!cmd_emptybuffer(buffer, "DBUFFER")) return done_error;

/* If buffer is the only buffer, set it up as empty; otherwise,
select another buffer if it is current, and then wipe it out. */

if (buffer == main_bufferchain && buffer->next == NULL)
  {
  init_buffer(buffer, 0, store_copystring(""), store_copystring(""), NULL, NULL,
    default_rmargin);
  currentbuffer = NULL;
  init_selectbuffer(buffer, FALSE);
  }

else
  {
  bufferstr *next = buffer->next;
  if (next == NULL) next = main_bufferchain;
  if (buffer == currentbuffer) init_selectbuffer(next, FALSE);
  *link = buffer->next;          /* disconnect from chain */
  store_free(buffer);
  if (main_bufferchain->next == NULL) main_drawgraticules |= dg_both;
  }

return done_continue;
}


/*************************************************
*                The DCUT command                *
*************************************************/

int e_dcut(cmdstr *cmd)
{
while (cut_buffer != NULL)
  {
  linestr *next = cut_buffer->next;
  store_free(cut_buffer->text);
  store_free(cut_buffer);
  cut_buffer = next;
  }
cut_last = NULL;
cut_pasted = TRUE;
return done_continue;
}


/*************************************************
*              The DEBUG command                 *
*************************************************/

int e_debug(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) switch(cmd->arg1.value)
  {
  case debug_crash:
  *((int *)(-1)) = 0;
  break;

  case debug_exceedstore:
  (void) store_Xget(1000*1000*1000);
  break;

  case debug_nullline:
  main_current = NULL;
  break;

  case debug_baderror:
  error_moan(4, "Cause disastrous error", "debug command", 0, 0, 0, 0, 0);
  break;
  }
else error_printf("Warning! Careless use of the debug command can damage your data\n");

return done_wait;
}


/*************************************************
*              The DETRAIL command               *
*************************************************/

/* With no argument, removes trailing spaces from the current buffer,
first loading in the rest of the file if necessary. With "output" as
argument, sets flag for detrailing on output. */

int e_detrail(cmdstr *cmd)
{
if (cmd->misc == detrail_buffer)
  {
  linestr *line = main_top;
  while (file_extend() != NULL);
  while ((line->flags & lf_eof) == 0)
    {
    char *s = line->text;
    char *t = s + line->len;
    while (t > s && t[-1] == ' ') t--;
    if (t - s < line->len) { line->len = t - s; main_filechanged = TRUE; }
    line = line->next;
    }
  }
else main_detrail_output = TRUE;
return done_continue;
}



/*************************************************
*              The DF command                    *
*************************************************/

int e_df(cmdstr *cmd)
{
linestr *start = main_current;
int yield = e_f(cmd);
if (yield != done_continue) return yield;
while (start != main_current)
  {
  start = line_delete(start, TRUE);
  main_currentlinenumber--;
  }
cmd_recordchanged(main_current, cursor_col);
cmd_refresh = TRUE;
return done_continue;
}



/*************************************************
*                The DREST command               *
*************************************************/

int e_drest(cmdstr *cmd)
{
if ((main_bottom->flags & lf_eof) == 0)
  {
  store_free(main_bottom->text);
  main_bottom->text = NULL;
  main_bottom->len = 0;
  main_bottom->flags |= lf_eof;
  }
main_bottom->flags |= lf_shn;

scrn_hint(sh_topline, 0, NULL);

while ((main_current->flags & lf_eof) == 0)
  main_current = line_delete(main_current, FALSE);

if (from_fid != NULL)
  {
  fclose(from_fid);
  from_fid = NULL;
  }

cmd_recordchanged(main_current, cursor_col);
cmd_refresh = TRUE;

return done_continue;
}


/*************************************************
*          The DTA and DTB commands              *
*************************************************/

int e_dtab(cmdstr *cmd)
{
int oldcol = cursor_col;
int yield = e_pab(cmd);
if (yield == done_continue)
  {
  int count = cursor_col - oldcol;
  line_deletech(main_current, cursor_col, count, FALSE);
  main_current->flags |= lf_shn;
  cursor_col -= count;
  }
return yield;
}



/*************************************************
*          DTWL & DTWR (delete to word)          *
*************************************************/

int e_dtwl(cmdstr *cmd)
{
char *p = main_current->text;
int len = main_current->len;
int oldcursor = cursor_col;
int count;

if (cursor_col == 0) return done_continue;

if ((main_current->flags & lf_eof) != 0)
  {
  error_moan(30, "End of file", "dtwl");
  return done_error;
  }

if (cursor_col >= len) cursor_col = len;
while (--cursor_col > 0 && 
  (ch_tab[(unsigned int)(p[cursor_col])] & ch_word) == 0);
while (cursor_col > 0 && 
  (ch_tab[(unsigned int)(p[cursor_col])] & ch_word) != 0) cursor_col--;

if ((ch_tab[(unsigned int)(p[cursor_col])] & ch_word) == 0) cursor_col++;
count = oldcursor - cursor_col;
if (count > 0)
  {
  line_deletech(main_current, cursor_col, count, TRUE);
  main_current->flags |= lf_shn;
  }
return done_continue;
}


int e_dtwr(cmdstr *cmd)
{
char *p = main_current->text;
int len = main_current->len;
int col = cursor_col;
int count;

if ((main_current->flags & lf_eof) != 0)
  {
  error_moan(30, "End of file", "dtwr");
  return done_error;
  }

if (col >= len) col = len;
while (col < len && (ch_tab[(unsigned int)(p[col])] & ch_word) != 0) col++;
while (col < len && (ch_tab[(unsigned int)(p[col])] & ch_word) == 0) col++;

count = col - cursor_col;
if (count > 0)
  {
  line_deletech(main_current, cursor_col, count, TRUE);
  main_current->flags |= lf_shn;
  }
return done_continue;
}



/*************************************************
*           The EIGHTBIT command                 *
*************************************************/

int e_eightbit(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) main_eightbit = cmd->arg1.value;
  else main_eightbit = !main_eightbit;
screen_forcecls = TRUE;
return done_continue;
}



/*************************************************
*         The ENDPAR command                     *
*************************************************/

int e_endpar(cmdstr *cmd)
{
cmd_freeblock((cmdblock *)par_end);
par_end = cmd_copyblock((cmdblock *)cmd->arg1.se);
return done_continue;
}

/* End of c.ee1 */
