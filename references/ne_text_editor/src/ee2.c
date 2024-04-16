/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1999 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: July 1999 */


/* This file contains code for obeying commands: Part II */


#include "ehdr.h"
#include "cmdhdr.h"
#include "shdr.h"


/*************************************************
*          The F & BF commands                   *
*************************************************/

static int search(cmdstr *cmd, BOOL leftflag)
{
linestr *line = main_current;
sestr *se = cmd->arg1.se;
BOOL stringsearch;
BOOL matched = FALSE;
int linenumber = main_currentlinenumber;
int USW = 0;

match_L = leftflag;
if (!cmd_casematch) USW |= qsef_U;

/* Deal with saving and repeated search */

if ((cmd->flags & cmdf_arg1) != 0)
  {
  if (last_se != NULL) cmd_freeblock((cmdblock *)last_se);
  last_se = cmd_copyblock((cmdblock *)se);
  }
else if (last_se == NULL)
  {
  error_moan(16, "search command");
  return done_error;
  }
else se = last_se;

stringsearch = (se->type == cb_qstype) && ((se->flags & qsef_N) == 0);

/* If the cursor is not at the end (start) of the
current line, try to match on it, unless this is a
line search, in which case match only if the cursor
is at the start (end). */

if (match_L)
  {
  if (cursor_col > 0 && (line->flags & lf_eof) == 0 &&
    (stringsearch || cursor_col >= line->len))
      {
      match_leftpos = 0;
      match_rightpos = cursor_col;
      matched = cmd_matchse(se, line, USW);
      }
  }

else
  {
  if (cursor_col < line->len && (stringsearch || cursor_col == 0))
    {
    match_leftpos = cursor_col;
    match_rightpos = line->len;
    matched = cmd_matchse(se, line, USW);
    }
  }

/* Search for line */

match_leftpos = 0;
if ((line->flags & lf_eof) == 0 || match_L)
  {
  while (!matched)
    {
    if (main_interrupted(ci_move)) return done_error;
    if (match_L)
      {
      line = line->prev;
      linenumber--;
      }
    else
      {
      line = mac_nextline(line);
      linenumber++;
      }
    if (line == NULL) break;
    if ((line->flags & lf_eof) != 0)break;
    match_rightpos = line->len;
    matched = cmd_matchse(se, line, USW);
    }
  }

/* NOT matched => reached end (start) of file */

if (matched)
  {
  main_current = line;
  main_currentlinenumber = linenumber;
  cursor_col = match_L? match_start : match_end;
  cmd_saveback();
  return done_continue;
  }
else
  {
  if (cmd_eoftrap && !match_L) return done_eof;
  error_moanqse(17, se);
  return done_error;
  }
}


int e_bf(cmdstr *cmd) { return search(cmd, TRUE); }
int e_f(cmdstr *cmd)  { return search(cmd, FALSE); }



/*************************************************
*           The FKEYSTRING command               *
*************************************************/

int e_fks(cmdstr *cmd)
{
key_setfkey(cmd->arg1.value, ((cmd->flags & cmdf_arg2) == 0)? NULL :
  cmd->arg2.string->text);
return done_continue;
}


/*************************************************
*            The FORMAT command                  *
*************************************************/

int e_format(cmdstr *cmd)
{
if ((main_current->flags & lf_eof) == 0) 
  {
  line_formatpara();
  cmd_refresh = TRUE;
  }  
return done_continue;
}


/*************************************************
*          The GA, GB and GE commands            *
*************************************************/

/* They all call the same routine */

int e_g(cmdstr *cmd)
{
linestr *limitline = (mark_type == mark_global)? mark_line : NULL;
int resetgraticules = dg_none;
int lastr = 0;
int misc = cmd->misc;
int USW = 0;
int yield = done_continue;
int currentlinenumber = main_currentlinenumber;
int oldcurrentlinenumber = main_currentlinenumber;
int oldrmargin = main_rmargin;
int oldcursor = cursor_col;
int matchcount = 0;
int changecount = 0;
int rcount = 0;
char *wordptr = "";
BOOL lastchanged = FALSE;
BOOL all = !main_interactive;
BOOL change = all;
BOOL stringsearch, REreplace;
BOOL Gcontinue = TRUE;
BOOL matched = FALSE;
BOOL quit = FALSE;
BOOL skip_end = FALSE;
BOOL interrupted = FALSE;
linestr *line = main_current;
linestr *oldcurrent = line;
sestr *se = cmd->arg1.se;
qsstr *nt = cmd->arg2.qs;

/* Deal with saving arguments or re-using old ones */

if ((cmd->flags & cmdf_arg1) != 0)
  {
  if (last_gse != NULL) cmd_freeblock((cmdblock *)last_gse);
  if (last_gnt != NULL) cmd_freeblock((cmdblock *)last_gnt);
  last_gse = cmd_copyblock((cmdblock *)se);
  last_gnt = cmd_copyblock((cmdblock *)nt);
  }
else if (last_gse == NULL)
  {
  error_moan(16, "global command");
  return done_error;
  }
else
  {
  se = last_gse;
  nt = last_gnt;
  }

/* If search for null string, treat always as GB command
also check and complain if B or E qualifier is missing -
compile checks for whole commands, but interactive input
bypasses that check. (Interactive input not implemented
yet...)

If null string search is at end of line and the S qualifier
is present, remember this in order always to skip to the
end of line after a match. */

if (se->type == cb_qstype && ((qsstr *)se)->length == 0)
  {
  qsstr *q = (qsstr *)se;
  misc = abe_b;

  if ((q->flags & (qsef_B+qsef_E)) == 0)
    {
    error_moan(27);
    return done_error;
    }
    
  if ((q->flags & (qsef_E+qsef_S)) == qsef_E+qsef_S) skip_end = TRUE;  
  }

/* Final preparations */

stringsearch = (se->type == cb_qstype) &&
  ((((qsstr *)se)->flags & qsef_N) == 0);
REreplace = (nt->flags & qsef_R) != 0;

/* No journal for PCRE regex */

if (stringsearch && (((qsstr *)se)->flags & (qsef_R|qsef_RP)) == qsef_R)
  R_Journal = store_Xget(JournalSize);

match_L = FALSE;
if (!cmd_casematch) USW |= qsef_U;
if (main_rmargin < MAX_RMARGIN) main_rmargin += MAX_RMARGIN;
cmd_saveback();

/* Main loop starts here */

while (Gcontinue)
  {
  matched = FALSE;

  /* Look at current line if relevant */

  if (cursor_col < line->len && (stringsearch || cursor_col == 0) &&
    (matchcount == 0 || (se->flags & qsef_B) == 0))  /* for null replacements */
    {
    if (main_interrupted(ci_move))
      {
      yield = done_error;
      quit = interrupted = TRUE;
      break;
      }
    match_leftpos = cursor_col;
    match_rightpos = line->len;
    if (line == limitline && mark_col >= cursor_col)
      match_rightpos = mark_col;
    matched = cmd_matchse(se, line, USW);
    }

  /* Now look at subsequent lines */

  match_leftpos = 0;
  if ((line->flags & lf_eof) == 0) while (!matched)
    {
    if (main_interrupted(ci_move))
      {
      yield = done_error;
      quit = interrupted = TRUE;
      break;
      }
    if (line == limitline) break;
    line = mac_nextline(line);
    currentlinenumber++;
    if (line == NULL || (line->flags & lf_eof) != 0) break;

    match_rightpos = line->len;
    if (line == limitline) match_rightpos = mark_col;
    matched = cmd_matchse(se, line, USW);
    }

  /* Take action according as matched or not */

  if (matched)
    {
    int boldcol = match_start;
    int boldcount = match_end - match_start;
    if (boldcount == 0) boldcount = 1;
    matchcount++;
    main_current = line;
    main_currentlinenumber = currentlinenumber;
    #ifdef Windowing 
    if (main_windowing) cursor_row = main_currentlinenumber;
    #endif 
    if (misc == abe_a) cursor_col = match_end - 1;
      else cursor_col = match_start;
    if (cursor_col > oldrmargin) resetgraticules = dg_both;

    /* If interactive, prompt user, allowing multiple responses */

    if (main_interactive && !all)
      {
      char *prompt = "Change, Skip, Once, Last, All, Finish, Quit or Error? ";

      /* If nothing left from last prompt, read some more */
      if (*wordptr == 0 && rcount <= 0)
        {
        BOOL done = FALSE;
        while (!done)
          {
          if (main_screenOK)
            {
            scrn_hint(sh_above, 1, NULL);
            if (boldcol + boldcount >= cursor_max)
              {
              cursor_rh_adjust = cursor_max - boldcol + 1;
              if (cursor_rh_adjust > 20) cursor_rh_adjust = 20;
              }   
            if (cursor_rh_adjust < 3) cursor_rh_adjust = 3; 
            scrn_display(TRUE);
            cursor_rh_adjust = 0; 
            if (boldcol + boldcount > cursor_max) boldcount = cursor_max - boldcol; 
            scrn_invertchars(main_current, cursor_row, boldcol, boldcount, TRUE);
            scrn_rdline(FALSE, FALSE, prompt);
            scrn_display(FALSE);
            scrn_invertchars(main_current, cursor_row, boldcol, boldcount, FALSE);
            main_pendnl = TRUE;
            main_nowait = FALSE;
            s_selwindow(message_window, 0, 0);
            s_flush(); 
            s_cls();
            }
          else
            {
            line_verify(main_current, TRUE, TRUE);
            error_printf(prompt);
            error_printflush();  
            fgets(cmd_buffer, cmd_buffer_size, cmdin_fid);
            cmd_buffer[strlen(cmd_buffer)-1] = 0;
            }

          wordptr = cmd_buffer;
          while (*wordptr != 0)
            {
            int c = tolower(*wordptr);
            if (c != ' ')
              {
              if (isdigit(c) ||
                c=='c' || c=='o' || c=='a' || c=='f' || c=='e' || c=='q' || c=='s' || c=='l')
                  { done = TRUE; *wordptr++ = c; }
              else
                {
                #ifdef Windowing
                if (main_windowing) sys_werr("Respond with initial letters only");
                else 
                #endif
                  prompt = "Initial letters only: Change, Skip, "
                    "Once, Last, All, Finish, Quit or Error? ";
                done = FALSE;
                break;
                }
              }
            }
          }
        wordptr = cmd_buffer;
        rcount = 0;
        }

      /* Extract the next response */

      if (rcount > 0) rcount--; else
        {
        lastr = *wordptr++;
        while (isdigit(lastr))
          {
          rcount = rcount*10 + lastr - '0';
          lastr = *wordptr++;
          }
        rcount--;
        }

      /* Set flags according to response */

      change = FALSE;
      if (lastr == 'c') change = TRUE;
      else if (lastr == 'o') { change = TRUE; Gcontinue = FALSE; }
      else if (lastr == 'l')
        { change = TRUE; Gcontinue = FALSE; quit = TRUE; }
      else if (lastr == 'a') { change = TRUE; all = TRUE; }
      else if (lastr == 'f') Gcontinue = FALSE;
      else if (lastr == 'q') { Gcontinue = FALSE; quit = TRUE; }
      else if (lastr == 'e') { Gcontinue = FALSE; yield = done_error; }
      /* For 's' (skip), no action */
      }

    /* Make the change if wanted */

    if (change)
      {
      char *p = nt->text + 1;
      int len = nt->length;
      lastchanged = TRUE;
      changecount++;

      /* Regular Expression Change: if required wild
      strings are unavailable, give error and exit. */

      if (REreplace)
        {
        int ok;
        line = cmd_ReChange(line, p, len, (nt->flags & qsef_X) != 0,
          misc == abe_e, misc == abe_a, &ok);
        if (!ok)
          {
          error_moan(39);
          Gcontinue = FALSE;
          quit = TRUE;
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
          line_deletech(line, match_start, match_end-match_start, TRUE);
          line_insertch(line, match_start, p, len, 0);
          cursor_col = match_start + len;
          }
        else
          {
          line_insertch(line, ((misc == abe_a)? match_end : match_start), p, len, 0);
          cursor_col = match_end + len;
          }
        
        /* If the match included the qualifiers E and S, we might not actually be at
        the end of the line (because of trailing spaces) but we don't want to carry
        on operating on this line. */
         
        if (skip_end) cursor_col = line->len;
        }

      /* Note line has changed */
       
      line->flags |= lf_shn;

      /* Why was this code put in here? It causes much screen flapping during "all".
      Ah, perhaps wanted to see changes during other responses. Try cutting it out
      for "all". */  
       
      if (main_screenOK && !all)
        {
        scrn_display(TRUE);
        main_pendnl = TRUE;
        main_nowait = FALSE;
        s_selwindow(message_window, 0, 0);
        s_cls();
        }
      }

    else
      {
      lastchanged = FALSE;
      cursor_col = match_end;
      }
    }

  /* No match has been found */

  else
    {
    Gcontinue = FALSE;
    if (main_interactive && cmdin_fid == NULL && matchcount <= 0)
      {
      error_moanqse(17, se);  /* not found */
      yield = done_error;
      }
    }
  }

/* Verify number of matches and changes if interactive */

#ifdef Windowing
if (!interrupted && main_interactive && !main_windowing && matchcount > 0)
#else
if (!interrupted && main_interactive && matchcount > 0)
#endif
  {
  char buff[256];
  sprintf(buff, "%s%d match%s, %d change%s",
    (!matched && yield != done_error)? "No more: " : "",
      matchcount, (matchcount == 1)? "" : "es",
        changecount, (changecount == 1)? "" : "s");

  if (main_screenOK)
    {
    s_selwindow(message_window, 0, 0);
    s_cls();
    main_leave_message = TRUE; 
    if (mark_type == mark_unset)
      {
      s_printf("%s", buff);
      main_pendnl = TRUE;
      }
    else error_printf("%s\n", buff);
    }
  else sys_mprintf(msgs_fid, "%s\n", buff);
  }


/* Restore rmargin and original position unless "quit";
also if not "quit", record for the BACK command. */

main_rmargin = oldrmargin;
if (!quit)
  {
  cmd_saveback();
  cursor_col = oldcursor;
  main_current = oldcurrent;
  main_currentlinenumber = oldcurrentlinenumber;
  #ifdef Windowing
  if (main_windowing) cursor_row = main_currentlinenumber;
  #endif   
  }

if (R_Journal != NULL) { store_free(R_Journal); R_Journal = NULL; }
main_drawgraticules |= resetgraticules;
return yield;
}



/*************************************************
*            The HELP command                    *
*************************************************/

int e_help(cmdstr *cmd)
{
char *s = NULL;
if (!main_interactive) return done_continue;
if ((cmd->flags & cmdf_arg1) != 0) s = cmd->arg1.string->text;
if (!sys_help(s))
  {
  if (s == NULL)
    error_moan(42, "", "");
      else error_moan(42, " on ", s);
  return done_error;
  }
return done_wait;                 /* indicate output produced */
}



/*************************************************
*            The I command                       *
*************************************************/

/* Insert a whole file */

static int ifile(cmdstr *cmd)
{
FILE *f;
int binoffset = 0;
linestr *botline;
char *s;
char *name = cmd->arg1.string->text;

if ((s = sys_checkfilename(name, TRUE)) != NULL)
  {
  error_moan(12, name, s); 
  return done_error;
  }  

f = sys_fopen(name, "r", NULL);
if (f == NULL)
  {
  error_moan(5, name);
  return done_error;
  }

/* We first read all the lines into store, chaining
them together. Then we splice the chain into the
existing chain of lines above the current line.
Anomalously, the "back" point is the start of the
inserted data. */

botline = file_nextline(&f, &binoffset, NULL);

if ((botline->flags & lf_eof) == 0)
  {
  int count = 1;
  linestr *prev = main_current->prev;
  linestr *topline = botline;
  linestr *line = botline;

  botline = file_nextline(&f, &binoffset, NULL);
  while ((botline->flags & lf_eof) == 0)
    {
    line->next = botline;
    botline->prev = line;
    line = botline;
    botline = file_nextline(&f, &binoffset, NULL);
    count++;
    }

  line->next = main_current;
  topline->prev = prev;
  if (prev == NULL) main_top = topline;
    else prev->next = topline;
  main_current->prev = line;
  main_currentlinenumber += count;
  main_linecount += count;
  
  cmd_recordchanged(main_current, cursor_col);
  cmd_saveback();
  cmd_recordchanged(topline, 0);
  cmd_saveback(); 

  if (main_screenOK) scrn_hint(sh_insert, count, NULL);
  }

store_free(botline->text);
store_free(botline);
return done_continue;
}


static int ilines(cmdstr *cmd)
{
int count = 0;
int yield = done_continue;
linestr *prev = main_current->prev;

#ifdef Windowing
if (main_screenOK && !main_windowing)
#else
if (main_screenOK)
#endif
  {
  if (main_pendnl)
    {
    sys_mprintf(msgs_fid, "\r\n");
    main_pendnl = main_nowait = FALSE;
    }
  screen_forcecls = TRUE;
  }

/* Loop reading lines until teminator */

for (;;)
  {
  BOOL eof = FALSE;
  linestr *line;

  if (cmd_cbufferline != NULL)
    {
    line = store_copyline(cmd_cbufferline);
    cmd_cbufferline = cmd_cbufferline->next;
    cmd_clineno++; 
    }  
  else if (cmdin_fid != NULL) 
    {
    line = file_nextline(&cmdin_fid, NULL, NULL); 
    cmd_clineno++;
    }  
  else
    {
    int len;
    if (main_screenOK)
      {
      scrn_rdline(TRUE, FALSE, "NE< ");
      len = strlen(cmd_buffer);
      if (len > 0 && cmd_buffer[len-1] == '\n') cmd_buffer[--len] = 0;
      line = store_getlbuff(len);
      memcpy(line->text, cmd_buffer, len);
      }
    else line = file_nextline(&kbd_fid, NULL, NULL);
    }

  if ((line->flags & lf_eof) != 0)
    {
    error_moan(29, "End of file", "I");
    eof = TRUE;
    yield = done_error;
    }

  if (eof || main_interrupted(ci_read) ||
    (line->len == 1 && tolower(line->text[0]) == 'z'))
      {
      store_free(line->text);
      store_free(line);
      break;
      }

  if (prev == NULL) main_top = line; else prev->next = line;
  line->next = main_current;
  line->prev = prev;
  main_current->prev = prev = line;
  count++;
  }

main_currentlinenumber += count;
main_linecount += count;
cmd_recordchanged(main_current, cursor_col);
cmd_saveback();
if (main_screenOK) scrn_hint(sh_insert, count, NULL);
return yield;
}


int e_i(cmdstr *cmd)
{
cmd_refresh = TRUE;
if ((cmd->flags & cmdf_arg1) != 0) return ifile(cmd);
  else return ilines(cmd);
}



/*************************************************
*            The ICURRENT command                *
*************************************************/

int e_icurrent(cmdstr *cmd)
{
linestr *newline;
linestr *prev = main_current->prev;

if ((main_current->flags & lf_eof) != 0)
  {
  error_moan(29, "End of file", "ICURRENT");
  return done_error;
  }

newline = line_copy(main_current);
if (prev == NULL) main_top = newline; else prev->next = newline;
newline->prev = prev;
newline->next = main_current;
main_current->prev = newline;
main_currentlinenumber++;
main_linecount++;
cmd_recordchanged(main_current, cursor_col);
cmd_saveback();
if (main_screenOK) scrn_hint(sh_insert, 1, NULL);
cmd_refresh = TRUE;
return done_continue;
}



/*************************************************
*            The IF command                      *
*************************************************/

/* This procedure is also used for UNLESS -- the switch
in the command block distinguished. An absent first
argument signifies that the test is for eof or for the
marked line. */

int e_if(cmdstr *cmd)
{
BOOL match;
int misc = cmd->misc;
sestr *se = ((cmd->flags & cmdf_arg1) == 0)? NULL : cmd->arg1.se;
ifstr *ifblock = cmd->arg2.ifelse;

if ((misc & if_prompt) != 0)
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

return cmd_obeyline(match? ifblock->if_then : ifblock->if_else);
}



/*************************************************
*                The ILINE command               *
*************************************************/

int e_iline(cmdstr *cmd)
{
int len;
qsstr *qs = cmd->arg1.qs;
linestr *prev = main_current->prev;
linestr *line;
char *p, *pqs;

if ((qs->flags & qsef_X) == 0)
  {
  pqs = qs->text + 1;
  len = qs->length;
  }
else
  {
  pqs = qs->hexed;
  len = qs->length / 2;
  }

line = store_getlbuff(len);
p = line->text;
memcpy(p, pqs, len);
line->len = len;
line->next = main_current;
line->prev = prev;
main_current->prev = line;
if (prev == NULL) main_top = line; else prev->next = line;

main_linecount++;
cmd_recordchanged(main_current, cursor_col);
cmd_saveback();
if (main_screenOK) scrn_hint(sh_insert, 1, NULL);
cmd_refresh = TRUE;
return done_continue;
}



/*************************************************
*             The ISPACE command                 *
*************************************************/

int e_ispace(cmdstr *cmd)
{
if (mark_type != mark_rect)
  {
  error_moan(41, "ispace");
  return done_error;
  }
else
  {
  linestr *line, *endline;
  int left, right, rectwidth;

  if (cursor_col < mark_col)
    { left = cursor_col; right = mark_col; }
  else
    { left = mark_col; right = cursor_col; }

  if (line_checkabove(mark_line) >= 0)
    { line = mark_line; endline = main_current; }
  else
    { line = main_current; endline = mark_line; }

  rectwidth = right - left;
  mark_type = mark_unset;
  mark_line = NULL;

  /* Loop through the relevant lines */

  for (;;)
    {
    if ((line->flags & lf_eof) == 0)
      {
      line_insertch(line, left, NULL, 0, rectwidth);
      line->flags |= lf_shn;
      }
    if (line == endline) break;
    line = line->next;
    }
  return done_continue;
  }
}



/*************************************************
*                The KEY command                 *
*************************************************/

int e_key(cmdstr *cmd)
{
return key_set(cmd->arg1.string->text, TRUE)? done_continue : done_error;
}



/*************************************************
*            The LCL & UCL commands              *
*************************************************/

static int lettercase(int (*func)(int))
{
int i;
int len = main_current->len;
char *p = main_current->text;
for (i = cursor_col; i < len; i++) p[i] = func(p[i]);
if (cursor_col < len) cursor_col = len;
main_current->flags |= lf_shn;
main_filechanged = TRUE;
return done_continue;
}

int e_lcl(cmdstr *cmd) { return lettercase(tolower); }
int e_ucl(cmdstr *cmd) { return lettercase(toupper); }


/*************************************************
*             The LOAD command                   *
*************************************************/

int e_load (cmdstr *cmd)
{
#ifdef Windowing
char *windowtitle = currentbuffer->windowtitle;
int windowhandle = currentbuffer->windowhandle;
#endif

char *s = (cmd->arg1.string)->text;
bufferstr *buffer = currentbuffer;
bufferstr *next = currentbuffer->next;
int n = currentbuffer->bufferno;
int yield = done_continue;
int filetype;
BOOL noprompt = currentbuffer->noprompt || !main_warnings;
FILE *fid;

/* Emptybuffer does not free the windowtitle field */

if (!cmd_emptybuffer(currentbuffer, "LOAD")) return done_error;

if ((fid = sys_fopen(s, "r", &filetype)) == NULL)
  {
  error_moan(5, s);
  yield = done_error;
  s = "";
  }

/* Re-initialize buffer (destroys the next and noprompt fields; also the
windowtitle and windowhandle fields; gets a new back list). */

init_buffer(currentbuffer, n, store_copystring(s), store_copystring(s),
 fid, NULL, default_rmargin);

currentbuffer->next = next;             /* restore */
currentbuffer->noprompt = noprompt;
currentbuffer->filetype = filetype;

currentbuffer = NULL;                   /* de-select to inhibit save */
init_selectbuffer(buffer, FALSE);

#ifdef Windowing
currentbuffer->windowtitle = windowtitle;      /* Restore things */
currentbuffer->windowhandle = windowhandle;
if (main_windowing) 
  {
  sys_window_load();
  sys_window_reshow();
  }  
#endif

return yield;
}



/*************************************************
*               The LOOP command                 *
*************************************************/

int e_loop(cmdstr *cmd)
{
cmd_breakloopcount = ((cmd->flags & cmdf_arg1) != 0)? cmd->arg1.value : 1;
return done_loop;
}

/* End of ee2.c */
