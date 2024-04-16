/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1997 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: December 1997 */


/* This file contains code for obeying commands: Part III */


#include "ehdr.h"
#include "cmdhdr.h"
#include "keyhdr.h"
#include "shdr.h"



/*************************************************
*            The M command                       *
*************************************************/

int e_m(cmdstr *cmd)
{
linestr *line = main_current;
int linenumber = main_currentlinenumber;
int found = FALSE;
int n = cmd->arg1.value;

/* Zero means top of file */

if (n == 0)
  {
  line = main_top;
  linenumber = 0;
  found = TRUE;
  }

/* Negative means bottom of file */

else if (n < 0)
  {
  while (file_extend() != NULL)
    if (main_interrupted(ci_move)) return done_error;
  line = main_bottom;
  linenumber = main_linecount - 1;
  found = TRUE;
  }

/* Otherwise seek a numbered line. We first have to discover in which
direction we need to move. */

else
  {
  /* Advance to a non-inserted line or the end of the file. */

  while (line->key <= 0)
    {
    if (main_interrupted(ci_move)) return done_error;
    if ((line->flags & lf_eof) != 0) break;
    line = mac_nextline(line);
    linenumber++;
    }

  /* Forwards search */

  if ((line->flags & lf_eof) == 0 && n > line->key) for (;;)
    {
    if (main_interrupted(ci_move)) return done_error;
    if (n == line->key) { found = TRUE; break; }
    if (n < line->key || (line->flags & lf_eof) != 0) break;
    line = mac_nextline(line);
    linenumber++;
    }

  /* Backwards search */

  else for (;;)
    {
    if (main_interrupted(ci_move)) return done_error;
    if (n == line->key) { found = TRUE; break; }
    if ((line->key > 0 && n > line->key) || line->prev == NULL) break;
    line = line->prev;
    linenumber--;
    }
  }

if (found)
  {
  main_current = line;
  main_currentlinenumber = linenumber;
  cursor_col = 0;
  cmd_saveback();
  return done_continue;
  }

else
  {
  error_moan(25, n);
  return done_error;
  }
}



/*************************************************
*            The MAKEBUFFER command              *
*************************************************/

int e_makebuffer(cmdstr *cmd)
{
bufferstr *old = currentbuffer;

/* Ensure a buffer with this number does not exist */

if (cmd_findbuffer(cmd->arg2.value) != NULL)
  {
  error_moan(51, cmd->arg2.value);
  return done_error;
  }

/* Create a new buffer (with an arbitrary number), and change
the number to the one required. The old number will have been
main_nextbufferno-1, so reset that. */

e_newbuffer(cmd);
currentbuffer->bufferno = cmd->arg2.value;
main_nextbufferno--;

/* The new buffer will have been made current; revert to the
previously current buffer. */

#ifdef Windowing
if (main_windowing) sys_window_selectbuffer(old); else
#endif
init_selectbuffer(old, FALSE);

return done_continue;
}




/*************************************************
*           The MARK command                     *
*************************************************/

int e_mark(cmdstr *cmd)
{
int misc = cmd->misc;
int yield = done_continue;

if (misc == amark_unset)
  {
  if (mark_type != mark_unset)
    {
    mark_line->flags |= lf_shn;
    mark_line = NULL;
    mark_type = mark_unset;
    }
  }
else
  {
  int type;
  char *name;

  if (misc == amark_limit)
    { type = mark_global; name = "Global"; }
  else if (misc == amark_line || misc == amark_hold)
    { type = mark_lines; name = "Line"; }
  else if (misc == amark_rectangle)
    { type = mark_rect; name = "Rectangle"; }
  else { type = mark_text; name = "Text"; }

  if (mark_type == mark_unset)
    {
    main_current->flags |= lf_shn;
    mark_line = main_current;
    mark_col = cursor_col;
    mark_type = type;
    mark_hold = misc == amark_hold;
    }

  else { error_moan(43, name); yield = done_error; }
  }

return yield;
}


/*************************************************
*            The N command                       *
*************************************************/

int e_n(cmdstr *cmd)
{
if ((main_current->flags & lf_eof) != 0)
  {
  if (main_eoftrap) return done_eof;
  error_moan(30, "end of file", "n");
  return done_error;
  }
else
  {
  main_current = mac_nextline(main_current);
  main_currentlinenumber++;
  cursor_col = 0;
  cmd_saveback();
  return done_continue;
  }
}



/*************************************************
*             The NAME command                   *
*************************************************/

int e_name(cmdstr *cmd)
{
char *s = (cmd->arg1.string)->text;
int len = strlen(s) + 1;

if (currentbuffer->to_fid != NULL)
  {
  error_moan(54, "\"name\"");
  return done_error;
  }   

store_free(main_filealias);
store_free(main_filename);

main_filealias = store_Xget(len);
main_filename = store_Xget(len);

strcpy(main_filealias, s);
strcpy(main_filename, s);

currentbuffer->filename = main_filename;
currentbuffer->filealias = main_filealias;

#ifdef Windowing
if (main_windowing) sys_window_showmark(mark_type, mark_hold);  /* For new name */
#endif

main_drawgraticules |= dg_bottom;
main_filechanged = TRUE;

/* If we have reached end of file, close the input, so as to
release the file lock in multi-user systems. If the entire file
has not yet been read, set a flag to cause closing when end of
file is reached. */

if (from_fid != NULL)
  {
  if ((main_bottom->flags & lf_eof) != 0)
    {
    fclose(from_fid);
    from_fid = NULL;
    currentbuffer->from_fid = NULL;
    }
  else currentbuffer->cleof = TRUE;
  }

return done_continue;
}



/*************************************************
*            The NEWBUFFER command               *
*************************************************/

int e_newbuffer(cmdstr *cmd)
{
char *name = NULL;
if ((cmd->flags & cmdf_arg1) != 0) name = (cmd->arg1.string)->text;
#ifdef Windowing
if (main_windowing)
  return sys_window_start_off(name)? done_continue : done_error; else
#endif
return setup_newbuffer(name);
}


/* Set up new buffer, given the name */

int setup_newbuffer(char *name)
{
bufferstr *new;
int filetype = default_filetype;
FILE *fid = NULL;

if (name != NULL && name[0] != 0)
  {
  fid = sys_fopen(name, "r", &filetype);
  if (fid == NULL)
    {
    error_moan(5, name);
    return done_error;
    }
  }

/* Get store for new buffer control data */

new = store_Xget(sizeof(bufferstr));

/* Ensure an unused buffer number */

while (cmd_findbuffer(main_nextbufferno++) != NULL);

/* Initialise the new buffer, chain it, and select it */

init_buffer(new, main_nextbufferno - 1, store_copystring(name),
  store_copystring(name), fid, NULL, main_rmargin);
new->filetype = filetype; 
new->next = main_bufferchain;
main_bufferchain = new;
init_selectbuffer(new, FALSE);
return done_continue;
}




/*************************************************
*           The OVERSTRIKE command               *
*************************************************/

int e_overstrike(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) main_overstrike = cmd->arg1.value;
  else main_overstrike = !main_overstrike;
if (main_screenOK)
  {
  s_overstrike(main_overstrike);
  main_drawgraticules |= dg_flags;
  }
return done_continue;
}


/*************************************************
*            The P command                       *
*************************************************/

int e_p(cmdstr *cmd)
{
if (main_current->prev == NULL)
  {
  error_moan(30, "start of file", "p");
  return done_error;
  }
else
  {
  main_current = main_current->prev;
  main_currentlinenumber--;
  cursor_col = 0;
  cmd_saveback();
  return done_continue;
  }
}



/*************************************************
*          The PA & PB commands                  *
*************************************************/

int e_pab(cmdstr *cmd)
{
int USW = 0;
if (!cmd_casematch) USW |= qsef_U;
match_L = FALSE;
match_leftpos = cursor_col;
match_rightpos = main_current->len;

if (cmd_matchse(cmd->arg1.se, main_current, USW))
  {
  cursor_col = (cmd->misc == abe_b)? match_start : match_end;
  return done_continue;
  }
else
  {
  error_moanqse(17, cmd->arg1.se);
  return done_error;
  }
}



/*************************************************
*          The PASTE command                     *
*************************************************/

int e_paste(cmdstr *cmd)
{
bufferstr *old = currentbuffer;

/* An argument specifies which window to paste in */

if ((cmd->flags & cmdf_arg1) != 0)
  {
  bufferstr *new = cmd_findbuffer(cmd->arg1.value);
  if (new == NULL)
    {
    error_moan(26, cmd->arg1.value);
    return done_error;
    }
  if (new != old)
    {
    #ifdef Windowing
    if (main_windowing) sys_window_selectbuffer(new); else
    #endif
    init_selectbuffer(new, FALSE);
    }
  }

if (cut_buffer != NULL)
  {
  if (cut_type == cuttype_text) cut_pastetext(); else cut_pasterect();
  }

/* Restore previous buffer */

if (currentbuffer != old)
  {
  #ifdef Windowing
  if (main_windowing) 
    {
    sys_window_display(FALSE, FALSE); 
    sys_window_selectbuffer(old); 
    } 
  else
  #endif
  init_selectbuffer(old, FALSE);
  } 

return done_continue;
}



/*************************************************
*            The PLL and PLR commands            *
*************************************************/

int e_plllr(cmdstr *cmd)
{
cursor_col = (cmd->misc == abe_b)? 0 : main_current->len;
return done_continue;
}




/*************************************************
*              The PROC command                  *
*************************************************/

int e_proc(cmdstr *cmd)
{
char *name = cmd->arg1.string->text;
if (cmd_findproc(name, NULL))
  {
  error_moan(45, name);
  return done_error;
  }
else
  {
  procstr *p = store_Xget(sizeof(procstr));
  p->type = cb_prtype;
  p->flags = 0;
  p->name = store_copystring(name);
  p->body = cmd_copyblock((cmdblock *)(cmd->arg2.cmds));
  p->next = main_proclist;
  main_proclist = p;
  return done_continue;
  }
}



/*************************************************
*             The PROMPT command                 *
*************************************************/

int e_prompt(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) currentbuffer->noprompt = !cmd->arg1.value;
  else currentbuffer->noprompt = !currentbuffer->noprompt;
return done_continue;
}


/*************************************************
*           The READONLY command                 *
*************************************************/

int e_readonly(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg1) != 0) main_readonly = cmd->arg1.value;
  else main_readonly = !main_readonly;
main_drawgraticules |= dg_flags;
return done_continue;
}


/*************************************************
*             The REFRESH command                *
*************************************************/

int e_refresh(cmdstr *cmd)
{
if (main_screenOK)
  {
  #ifdef Windowing
  if (main_windowing) sys_window_refresh(); else
  #endif
    {
    screen_forcecls = TRUE;
    scrn_display(FALSE);
    s_selwindow(message_window, -1, -1);
    s_cls();
    s_flush();
    main_pendnl = FALSE;
    }
  }
return done_continue;
}



/*************************************************
*              The RENUMBER command              *
*************************************************/

int e_renumber(cmdstr *cmd)
{
int number = 1;
linestr *line = main_top;

if (currentbuffer->to_fid != NULL)
  {
  error_moan(54, "\"renumber\"");
  return done_error;
  }   

for (;;)
  {
  line->key = number++;
  if ((line->flags & lf_eof) != 0) break;
  line = mac_nextline(line);
  }
return done_continue;
}



/*************************************************
*             The REPEAT command                 *
*************************************************/

int e_repeat(cmdstr *cmd)
{
int yield = done_loop;
while (yield == done_loop)
  {
  yield = done_continue;
  while (yield == done_continue)
    {
    if (main_interrupted(ci_loop)) return done_error;
    yield = cmd_obeyline(cmd->arg1.cmds);
    }
  if (yield == done_loop || yield == done_break)
    {
    if (--cmd_breakloopcount > 0) break;
    if (yield == done_break) yield = done_continue;
    }
  }
return yield;
}


/*************************************************
*             The RMARGIN command                *
*************************************************/

int e_rmargin(cmdstr *cmd)
{
if ((cmd->flags & cmdf_arg2) != 0)
  {
  if (cmd->arg2.value)
    {
    if (main_rmargin > MAX_RMARGIN) main_rmargin -= MAX_RMARGIN;
    }
  else if (main_rmargin < MAX_RMARGIN) main_rmargin += MAX_RMARGIN;
  main_drawgraticules |= dg_margin;
  }

else if ((cmd->flags & cmdf_arg1) != 0)
  {
  int r = cmd->arg1.value;
  if (r > 0)
    {
    main_rmargin = r;
    main_drawgraticules |= dg_both;
    }
  else
    {
    error_moan(15, "0", "as an argument for RMARGIN");
    return done_error;
    }
  }

else
  {
  if (main_rmargin > MAX_RMARGIN) main_rmargin -= MAX_RMARGIN; else
    main_rmargin += MAX_RMARGIN;
  main_drawgraticules |= dg_margin;
  }

return done_continue;
}


/*************************************************
*             The SA & SB commands               *
*************************************************/

int e_sab(cmdstr *cmd)
{
linestr *prevline = main_current;
int yield = e_pab(cmd);

if (yield != done_continue) return yield;
main_current->flags |= lf_shn;
main_current = line_split(main_current, cursor_col);
cursor_col = 0;
main_currentlinenumber++;

#ifdef Windowing
if (main_windowing) cursor_row = main_currentlinenumber;
#endif

if (main_AutoAlign)
  {
  int i;
  char *p = prevline->text;
  for (i = 0; i < prevline->len; i++)
    if (p[i] != ' ') { cursor_col = i; break; }
  if (cursor_col > 0)
    line_leftalign(main_current, cursor_col, &i);
  }

cmd_refresh = TRUE;
return done_continue;
}

/* End of ee3.c */
