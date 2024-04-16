/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1999 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: July 1999 */


/* This file contains code for the top-level handling of a command
line, and for compiling the line into an internal form. */


#include "ehdr.h"


static BOOL SpecialCmd;

/* This list must be in alphabetical order, and must be kept in step
with the two tables at the end of ecmdarg, and also with the following
table of read-only permissions. This table is global so that it can be
output by the SHOW command. */

char *cmd_list[] = {
  "a",
  "abandon",
  "align",
  "attn",
  "autoalign",
  "b",
  "back",
  "backup", 
  "beginpar",
  "bf",
  "break",
  "buffer",
  "c",
  "casematch",
  "cbuffer",
  "cdbuffer",
  "center",
  "centre",
  "cl",
  "closeback",
  "closeup",
  "comment",
  "copy",
  "cproc",
  "csd",
  "csu",
  "cut",
  "cutstyle",
  "dbuffer",
  "dcut",
  "debug",
  "detrail",
  "df",
  "dleft",
  "dline",
  "dmarked",
  "drest",
  "dright",
  "dta",
  "dtb",
  "dtwl",
  "dtwr",  
  "e",
  "eightbit", 
  "endpar",
  "f",
  "fkeystring", 
  "fks",
  "format",
  "ga",
  "gb",
  "ge",
  "help",
  "i",
  "icurrent",
  "if",
  "iline",
  "ispace",
  "key",
  "lcl",
  "load",
  "loop",
  "m",
  "makebuffer",
  "mark",
  "n",
  "name",
  "ne", 
  "newbuffer",
  "overstrike",
  "p",
  "pa",
  "paste",
  "pb",
  "pbuffer", 
  "pll",
  "plr",
  "proc",
  "prompt",
  "readonly", 
  "refresh",
  "renumber",
  "repeat",
  "rmargin",
  "sa",
  "save",
  "sb",
  "set",
  "show",
  "stop",
  "stream",
  "streammax",  
  "t",
  "title",
  "tl",
  "topline",
  "ucl",
  "undelete", 
  "unixregexp", 
  "unless",
  "until",
  "uteof",
  "verify",
  "w",
  "warn",
  "while",
  "word",
  "write"
};

int cmd_listsize = sizeof(cmd_list)/sizeof(char *);

/* The ids for special commands carry on from the end of those
for normal commands. */

#define cmd_specialbase cmd_listsize


/* Indicators for commands that are permitted in readonly buffers */

char cmd_readonly[] = {
  0, /* a */
  1, /* abandon */
  0, /* align */
  1, /* attn */
  1, /* autoalign */
  0, /* b */
  0, /* back */
  1, /* backup */ 
  1, /* beginpar */
  1, /* bf */
  1, /* break */
  1, /* buffer */
  1, /* c */
  1, /* casematch */
  1, /* cbuffer */
  1, /* cdbuffer */
  0, /* center */
  0, /* centre */
  0, /* cl */
  0, /* closeback */
  0, /* closeup */
  1, /* comment */
  1, /* copy */
  1, /* cproc */
  1, /* csd */
  1, /* csu */
  0, /* cut */
  1, /* cutstyle */
  1, /* dbuffer */
  1, /* dcut */
  1, /* debug */
  0, /* detrail */
  0, /* df */
  0, /* dleft */
  0, /* dline */
  0, /* dmarked */
  0, /* drest */
  0, /* dright */
  0, /* dta */
  0, /* dtb */
  0, /* dtwl */
  0, /* dtwr */  
  0, /* e */
  1, /* eightbit */ 
  1, /* endpar */
  1, /* f */
  1, /* fkeystring */
  1, /* fks */
  0, /* format */
  0, /* ga */
  0, /* gb */
  0, /* ge */
  1, /* help */
  0, /* i */
  0, /* icurrent */
  1, /* if */
  0, /* iline */
  0, /* ispace */
  1, /* key */
  0, /* lcl */
  1, /* load */
  1, /* loop */
  1, /* m */
  1, /* makebuffer */
  1, /* mark */
  1, /* n */
  1, /* name */
  1, /* ne */
  1, /* newbuffer */
  1, /* overstrike */
  1, /* p */
  1, /* pa */
  0, /* paste */
  1, /* pb */
  1, /* pbuffer */ 
  1, /* pll */
  1, /* plr */
  1, /* proc */
  1, /* prompt */
  1, /* readonly */ 
  1, /* refresh */
  0, /* renumber */
  1, /* repeat */
  1, /* rmargin */
  0, /* sa */
  1, /* save */
  0, /* sb */
  1, /* set */
  1, /* show */
  1, /* stop */
  1, /* stream */
  1, /* streammax */  
  1, /* t */
  1, /* title */
  1, /* tl */
  1, /* topline */
  0, /* ucl */
  0, /* undelete */ 
  1, /* unixregexp */ 
  1, /* unless */
  1, /* until */
  1, /* uteof */
  1, /* verify */
  1, /* w */
  1, /* warn */
  1, /* while */
  1, /* word */
  1  /* write */
};


/* Single-character special commands; we have star at the front of
the string to allocate it an id, though it is never matched via
this string. */

static char *xcmdlist = "*?><#$%~";

/* The id for a bracketed sequence of commands follows on the end */

#define cmd_specialend (cmd_specialbase + (int)strlen(xcmdlist))
#define cmd_sequence cmd_specialend
#define cmd_obeyproc (cmd_specialend+1)

/* Allow for mutual recursion between command sequence and command compile */

static cmdstr *CompileSequence(int *endscolon);


/*************************************************
*         Compile "system" command               *
*************************************************/

static cmdstr *CompileSysLine(void)
{
stringstr *string = store_Xget(sizeof(stringstr));
cmdstr *yield = cmd_getcmdstr(cmd_specialbase);     /* "*" is the first special */

yield->flags |= cmdf_arg1 + cmdf_arg1F;

string->type = cb_sttype;
string->delim = 0;
string->hexed = FALSE;
string->text = store_copystring(cmd_ptr + 1);

while (*cmd_ptr) cmd_ptr++;

yield->arg1.string = string;
return yield;
}




/*************************************************
*             Compile one command                *
*************************************************/

cmdstr *cmd_compile(void)
{
int found = -1;
int count = cmd_readnumber();
if (count < 0) count = 1;

SpecialCmd = FALSE;     /* indicates special one-character cmd */
cmd_ist = 0;            /* implicit string terminator */

/* Attempt to read a word; yields "" if non-letter found */

cmd_readword();

/* Cases where there is no word */

if (cmd_word[0] == 0)
  {
  if (cmd_atend() || (*cmd_ptr == '\\' && (main_oldcomment || cmd_ptr[1] == '\\'))) 
    return NULL;   /* End of line */

  /* Deal with bracketed groups */

  if (*cmd_ptr == '(')
    {
    int dummy;
    cmdstr *yield = cmd_getcmdstr(found);
    yield->count = count;
    cmd_ptr++;
    yield->id = cmd_sequence;

    cmd_bracount++;
    yield->arg1.cmds = CompileSequence(&dummy);
    yield->flags |= cmdf_arg1 | cmdf_arg1F;
    cmd_bracount--;

    if (!cmd_faildecode)
      {
      if (*cmd_ptr == ')') cmd_ptr++; else
        {
        error_moan(13, "\")\"");
        cmd_faildecode = TRUE;
        }
      }
    return yield;
    }

  /* Deal with procedure call */

  else if (*cmd_ptr == '.')
    {
    stringstr *name;
    if (cmd_readprocname(&name))
      {
      cmdstr *yield = cmd_getcmdstr(cmd_obeyproc);
      yield->count = count;
      yield->arg1.string = name;
      yield->flags |= cmdf_arg1 | cmdf_arg1F;
      return yield;
      }
    else
      {
      cmd_faildecode = TRUE;
      return NULL;
      }
    }

  /* Deal with special character commands */

  else
    {
    char *p;
    int c = *cmd_ptr++;
    SpecialCmd = TRUE;
    mac_skipspaces(cmd_ptr);

    /* Pack up multiples by using count */
    while (*cmd_ptr == c)
      {
      count++;
      cmd_ptr++;
      mac_skipspaces(cmd_ptr);
      }

    /* Now search for known command */

    if ((p = strchr(xcmdlist, c)) == NULL)
      {
      char s[2];
      s[0] = c;
      s[1] = 0;
      error_moan(10, s);
      cmd_faildecode = TRUE;
      return NULL;
      }

    else
      {
      cmdstr *yield = cmd_getcmdstr(found);
      yield->count = count;
      if (*cmd_ptr == '^') cmd_ptr++;
      yield->id = cmd_specialbase + p - xcmdlist;
      return yield;
      }
    }
  }

/* Else use binary chop to search command word list */

else
  {
  char **first = cmd_list;
  char **last  = first + cmd_listsize;

  while (last > first)
    {
    int c;
    char **cmd = first + (last-first)/2;
    c = strcmp(cmd_word, *cmd);
    if (c == 0)
      {
      found = (cmd - cmd_list);
      break;
      }
    if (c > 0) first = cmd + 1; else last = cmd;
    }
  }

/* If valid command word found, get a control block
and call the command-specific compiling routine to
read the arguments. Before doing this, we skip a single
'^' character in the input, if present. This character
can therefore be used as a delimiter in -opt and -init strings,
avoiding the need for quoting for spaces. */

if (found >= 0)
  {
  cmdstr *yield = cmd_getcmdstr(found);
  yield->count = count;
  if (*cmd_ptr == '^') cmd_ptr++;
  (cmd_Cproclist[found])(yield);
  return yield;
  }

else
  {
  if (strcmp(cmd_word, "else") == 0) error_moan(9);  /* misplaced else */
    else error_moan(10, cmd_word);                   /* unknown command */
  cmd_faildecode = TRUE;
  return NULL;
  }
}


/*************************************************
*       Compile Sequence until ')' or end        *
*************************************************/

static cmdstr *CompileSequence(int *endscolon)
{
cmdstr *yield = NULL;
cmdstr *last = NULL;
BOOL firsttime = TRUE;

SpecialCmd = TRUE;       /* no advance cmdptr first time */

/* Loop compiling commands - repeat condition is at the bottom */

do
  {
  *endscolon = FALSE;

  if (*cmd_ptr == ';' || SpecialCmd)
    {
    cmdstr *next;
    if (!SpecialCmd) cmd_ptr++;
    next = cmd_compile();
    mac_skipspaces(cmd_ptr);
    if (last == NULL) yield = next; else last->next = next;
    if (next == NULL)
      {
      if (!firsttime) *endscolon = TRUE;
      }
    else last = next;
    }
  else { error_moan(8); cmd_faildecode = TRUE; break; }

  /* Deal with the case of a logical command line
  extending over more than one physical command line. */

  while (!cmd_faildecode && cmd_bracount > 0 && (*cmd_ptr == 0 || *cmd_ptr == '\n' ||
    (*cmd_ptr == '\\' && (main_oldcomment || cmd_ptr[1] == '\\'))))
    {
    if (main_interrupted(ci_read)) { cmd_faildecode = TRUE; break; }
    cmd_joinline(FALSE);      /* join on next line, error if eof */
    }                         /* cmd_ptr will be at an initial semicolon */

  firsttime = FALSE;
  }
while (!cmd_faildecode && *cmd_ptr != 0 && *cmd_ptr != '\n' &&
  (*cmd_ptr != '\\' || (!main_oldcomment && cmd_ptr[1] != '\\')) && *cmd_ptr != ')');

return yield;
}



/*************************************************
*        Compile Command Line                    *
*************************************************/

/* The yield is a pointer to the first command block of the
compiled line, or NULL if the line is empty or in certain error
cases. The global variable faildecode must be set to indicate
success/failure. The global variable cmdline points to the line
being compiled. This may be changed by CompileSequence if the
line is continued onto subsequent physical input lines. The
argument endscolon is the address of a variable that must be set
to indicate whether the line ended in a semicolon or not. */

static cmdstr *CompileCmdLine(char *cmdline, BOOL *endscolon)
{
cmdstr *yield;
cmd_faildecode = FALSE;
cmd_cmdline = cmd_ptr = cmdline;
cmd_bracount = 0;

mac_skipspaces(cmd_ptr);
*endscolon = FALSE;

if (*cmd_ptr == '*') yield = CompileSysLine(); else
  {
  yield = CompileSequence(endscolon);
  if (*cmd_ptr != 0 && *cmd_ptr != '\n' && 
    (*cmd_ptr != '\\' || (!main_oldcomment && cmd_ptr[1] != '\\')) 
      && !cmd_faildecode)
    {
    error_moan(7);             /* unmatched ')' */
    cmd_faildecode = TRUE;
    }
  }

return yield;
}



/*************************************************
*                Obey command line               *
*************************************************/

/* Command line equals "sequence of commands in brackets" */

int cmd_obeyline(cmdstr *cmd)
{
int yield = done_continue;

if (cmd == NULL) return done_continue;

if (++cmd_bracount > 300)
  {
  error_moan(31);      /* nesting level too deep */
  return done_error;
  }

/* For each command, obey it <count> times, provided
there are no errors. Single-char commands take care
of the count themselves, so are called only once. They
are identified by high-valued ids. */

while (cmd != NULL && yield == done_continue)
  {
  int count = cmd->count;
  if ((int)(cmd->id) >= cmd_specialbase && (int)(cmd->id) < cmd_specialend) count = 1;
  if (main_readonly && !cmd_readonly[(unsigned int)(cmd->id)])
    {
    error_moan(52, cmd_list[(unsigned int)(cmd->id)]); 
    yield = done_error;
    break;  
    }  
  while (count-- > 0)
    {
    if (main_interrupted(ci_cmd)) return done_error;

    /* Now obey the command, maintaining the BACK flag (?). The main_leave_message
    flag is set if the command leaves a message in the message window in screen
    mode running. */

    main_leave_message = FALSE;
    yield = (cmd_Eproclist[(unsigned int)(cmd->id)])(cmd);

    /* Commands that generate output (e_g. SHOW) return
    done_wait; in fullscreen mode, if there are more commands
    to follow on the line, we take a pause here. Otherwise
    we pass back done_wait and the mainline will read more
    commands instead of reverting to full-screen. */

    if (yield == done_wait)
      {
      #ifdef Windowing
      if (main_screenOK && !main_windowing)
      #else
      if (main_screenOK)
      #endif
        {
        if (cmd_bracount == 1 && cmd->next == NULL && count == 0) break; else
          {
          yield = done_continue;
          scrn_rdline(TRUE, FALSE, "Press RETURN to continue ");
          error_printf("\n");
          }
        }
      #ifdef Windowing 
      else if (!main_screenmode || main_windowing) yield = done_continue;
      #else
      else if (!main_screenmode) yield = done_continue;
      #endif 
      }
    else if (yield != done_continue) break;
    }

/***** This wasn't present in the old E, and isn't right here.
We need to invent code to support break & loop in bracketed
groups that are repeated by repeat counts. Putting this here
unqualified applies them to individual commands. Have to
test for bracket groups??

  if (yield == done_loop || yield == done_break)
    {
    if (--cmd_breakloopcount > 0) break;
    if (yield == done_break) yield = done_continue;
    }
*****/

  /* Carry on with next command */

  cmd = cmd->next;
  }

cmd_bracount--;
return yield;
}



/*************************************************
*     Obey bracketed sequence of commands        *
*************************************************/

int e_sequence(cmdstr *cmd)
{
return cmd_obeyline(cmd->arg1.cmds);
}



/*************************************************
*               Handle command line              *
*************************************************/

/* The command line may be in fixed store, and so is not freed. */

int cmd_obey(char *cmdline)
{
int yield = done_error;
BOOL endscolon;
cmdstr *compiled;

main_cicount = 0;
if (main_interactive) sys_hourglass(TRUE);
compiled = CompileCmdLine(cmdline, &endscolon);

/* Save the command line, whether or not it compiled correctly, unless
it is null or identical to the previous line. */

if (cmdline[0] != 0 && (cmd_stackptr == 0 || strcmp(cmd_stack[cmd_stackptr-1], cmdline) != 0))
  {
  cmd_stack[cmd_stackptr++] = store_copystring(cmdline);
  if (cmd_stackptr > cmd_stacktop)
    {
    int i;
    store_free(cmd_stack[0]);
    for (i = 0; i < cmd_stacktop; i++) cmd_stack[i] = cmd_stack[i+1];
    cmd_stackptr--;
    }
  }

/* If successfully decoded, obey the commands and then free the store */

if (!cmd_faildecode)
  {
  cmd_onecommand = (compiled == NULL)? TRUE :
    (compiled->next == NULL && (compiled->flags & cmdf_group) == 0);
  cmd_bracount = 0;
  cmd_eoftrap = FALSE;
  cmd_refresh = FALSE;
  if ((yield = cmd_obeyline(compiled)) == done_finish) main_done = TRUE;
  cmd_freeblock((cmdblock *)compiled);
  
  /* There may be no current buffer in windowing mode if the last one has
  been closed down by the commands. */ 

  if (currentbuffer != NULL && currentbuffer->to_fid != NULL) main_checkstream();

  #ifdef Windowing
  if (main_windowing && cmd_refresh) sys_window_reshow();
  #endif
  }

if (main_interactive) sys_hourglass(FALSE);
return yield;
}

/* End of ecmdcomp.c */
