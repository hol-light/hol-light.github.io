/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1999 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: December 1999 */


/* This file contains code for command-processing functions */


#include "ehdr.h"


/*************************************************
*             Check for end of command           *
*************************************************/

BOOL cmd_atend(void)
{
mac_skipspaces(cmd_ptr);
return (*cmd_ptr == 0 || *cmd_ptr == ';' || *cmd_ptr == ')' || *cmd_ptr == '\n');
}


/*************************************************
*              Read a word                       *
*************************************************/

void cmd_readword(void)
{
int c;
int n = 0;
char *p = cmd_word;
mac_skipspaces(cmd_ptr);
c = *cmd_ptr;
while (isalpha(c))
  {
  if (n++ < max_wordlen) *p++ = tolower(c);
  if ((c = *(++cmd_ptr)) == 0) break;
  }
*p = 0;
}



/*************************************************
*            Read number                         *
*************************************************/

/* This procedure expects an unsigned decimal number.
A negative value is returned if one is not found. */

int cmd_readnumber(void)
{
int n = -1;
int c;
mac_skipspaces(cmd_ptr);

c = *cmd_ptr;
if (isdigit(c))
  {
  n = 0;
  while (isdigit(c))
    {
    n = n*10 + c - '0';
    c = *(++cmd_ptr);
    }
  }

return n;
}



/*************************************************
*        Read a Plain String                     *
*************************************************/

/* The argument is the address of where to put the
string control block pointer. The yield is negative
if there was a syntax error (bad delimiter), zero
if no string found (end of line or ';') and positive
if all was well. Delimiterless strings are also catered
for. */

static int xreadstring(stringstr **answer, int delimflag)
{
stringstr *st;
int dch;
char *p;

mac_skipspaces(cmd_ptr);
if ((delimflag < 0)? (*cmd_ptr == 0) : cmd_atend()) return 0;

if (delimflag == 0)
  {
  dch = *cmd_ptr++;
  if ((ch_tab[dch] & ch_delim) == 0)
    {
    error_moan(13, "String");
    return -1;
    }
  }
else dch = delimflag;

p = cmd_ptr;
while (*p != 0 && *p != '\n' && *p != dch) p++;
if ((*p == 0 || *p == '\n') && delimflag >= 0 && dch != ';') cmd_ist = dch;

st = store_Xget(sizeof(stringstr));
st->type = cb_sttype;
st->delim = dch;
st->hexed = FALSE;
st->text = store_copystring2(cmd_ptr, p);

cmd_ptr = (*p == 0 || *p == ';')? p : p+1;
mac_skipspaces(cmd_ptr);

*answer = st;  /* pass back control block */
return 1;      /* and appropriate result */
}


int cmd_readstring(stringstr **p)     { return xreadstring(p, 0); }
int cmd_readUstring(stringstr **p)    { return xreadstring(p, ';'); }
int cmd_readrestofline(stringstr **p) { return xreadstring(p, -1); }



/*************************************************
*        Get a command control block             *
*************************************************/

cmdstr *cmd_getcmdstr(int id)
{
cmdstr *yield = store_Xget(sizeof(cmdstr));
yield->flags = yield->misc = yield->ptype1 = yield->ptype2 = 0;
yield->next = NULL;
yield->type = cb_cmtype;
yield->id = id;
return yield;
}



/*************************************************
*            Copy a control block                *
*************************************************/

void *cmd_copyblock(cmdblock *cb)
{
void *yield;
if (cb == NULL) return NULL;
yield = store_copy(cb);

switch (cb->type)
  {
  case cb_sttype:
    {
    stringstr *y = (stringstr *)yield;
    stringstr *s = (stringstr *)cb;
    y->text = store_copy(s->text);
    }
  break;

  case cb_setype:
    {
    sestr *y = (sestr *)yield;
    sestr *s = (sestr *)cb;
    y->left.se = cmd_copyblock((cmdblock *)(s->left.se));
    y->right.se = cmd_copyblock((cmdblock *)(s->right.se));
    }
  break;

  case cb_qstype:
    {
    qsstr *y = (qsstr *)yield;
    qsstr *q = (qsstr *)cb;
    y->text  = store_copy(q->text);
    y->rtext = (q->rtext == q->text)? y->text : (char *)store_copy(q->rtext);
    y->fsm   = store_copy(q->fsm);
    y->hexed = store_copy(q->hexed);
    }
  break;

  case cb_cmtype:
    {
    cmdstr *cmd = (cmdstr *)cb;
    cmdstr *cy = (cmdstr *)yield;
    if ((cmd->flags & cmdf_arg1F) != 0)
      cy->arg1.cmds = (cmdstr *)cmd_copyblock((cmdblock *)cmd->arg1.cmds);
    if ((cmd->flags & cmdf_arg2F) != 0)
      cy->arg2.cmds = (cmdstr *)cmd_copyblock((cmdblock *)cmd->arg2.cmds);
    cy->next = cmd_copyblock((cmdblock *)cmd->next);
    }
  break;

  case cb_iftype:
    {
    ifstr *in  = (ifstr *)cb;
    ifstr *out = (ifstr *)yield;
    out->if_then = cmd_copyblock((cmdblock *)in->if_then);
    out->if_else = cmd_copyblock((cmdblock *)in->if_else);
    }
  break;

  case cb_prtype:
    {
    procstr *in =  (procstr *)cb;
    procstr *out = (procstr *)yield;
    out->name = store_copystring(in->name);
    out->body = cmd_copyblock((cmdblock *)in->body);
    }
  break;
  }

return yield;
}


/*************************************************
*            Free a control block                *
*************************************************/

/* Any attached blocks are also freed */

void cmd_freeblock(cmdblock *cb)
{
if (cb == NULL) return;

#ifdef showfree
printf("Freeing %d - ", cb);
#endif

switch (cb->type)
  {
  case cb_sttype:
    {
    stringstr *s = (stringstr *)cb;
    store_free(s->text);
    }
  break;

  case cb_setype:
    {
    sestr *s = (sestr *)cb;
    store_free(s->left.se);
    store_free(s->right.se);
    }
  break;

  case cb_qstype:
    {
    qsstr *q = (qsstr *)cb;
    store_free(q->text);
    if (q->rtext != q->text) store_free(q->rtext);
    store_free(q->fsm);
    store_free(q->hexed);
    }
  break;

  case cb_cmtype:
    {
    cmdstr *cmd = (cmdstr *)cb;
    if ((cmd->flags & cmdf_arg1F) != 0) cmd_freeblock((cmdblock *)cmd->arg1.block);
    if ((cmd->flags & cmdf_arg2F) != 0) cmd_freeblock((cmdblock *)cmd->arg2.block);
    cmd_freeblock((cmdblock *)cmd->next);
    }
  break;

  case cb_iftype:
    {
    ifstr *ifblock = (ifstr *)cb;
    cmd_freeblock((cmdblock *)ifblock->if_then);
    cmd_freeblock((cmdblock *)ifblock->if_else);
    }
  break;

  case cb_prtype:
    {
    procstr *pblock = (procstr *)cb;
    store_free(pblock->name);
    cmd_freeblock((cmdblock *)pblock->body);
    }
  break;
  }

store_free(cb);
}


/*************************************************
*         Join on continuation line              *
*************************************************/

/* The name is now a misnomer. It replaces rather than joins a new line.
We create a new command line starting with a semicolon. This is for the
benefit of the if command, which reads a new line to see if there is an "else",
and if not, wants to leave the pointer at a semicolon terminating the "if". */

int cmd_joinline(BOOL eofflag)
{
BOOL eof = FALSE;

/* Deal with command lines from buffer */

if (cmd_cbufferline != NULL)
  {
  if ((cmd_cbufferline->flags & lf_eof) != 0) eof = TRUE; else
    {
    if (cmd_cbufferline->len > cmd_buffer_size - 2)
       {
       error_moan(56);
       cmd_faildecode = TRUE;
       return FALSE;
       }
    if (cmd_cbufferline->len > 0)
      memcpy(cmd_buffer+1, cmd_cbufferline->text, cmd_cbufferline->len);
    cmd_buffer[cmd_cbufferline->len+1] = 0;
    cmd_cbufferline = cmd_cbufferline->next;
    cmd_clineno++;
    }
  }

/* Don't want to have yet another argument to scrn_rdline, so just move up
the line afterwards. */

else if (main_screenOK && cmdin_fid == NULL)
  {
  scrn_rdline(FALSE, FALSE, "NE+ ");
  memmove(cmd_buffer+1, cmd_buffer, cmd_buffer_size-1);
  main_nowait = main_repaint = TRUE;
  }
else
  {
  if (main_interactive && cmdin_fid != NULL)
    {
    error_printf("NE+ ");
    error_printflush();
    }
  eof = (cmdin_fid == NULL) || (fgets(cmd_buffer+1, cmd_buffer_size-1, cmdin_fid) == NULL);
  if (!eof) cmd_clineno++;
  }

/* Fill in the initial semicolon */

cmd_buffer[0] = ';';

/* Deal with reaching the end of a file */

if (eof)
  {
  if (!eofflag)     /* flag set => no error */
    {
    error_moan(32);
    cmd_faildecode = TRUE;
    }
  return FALSE;
  }

/* Set up for a new command line */

else
  {
  int n = strlen(cmd_buffer);
  if (n > 0 && cmd_buffer[n-1] == '\n') n--;
  if (cmd_ist > 0) cmd_buffer[n++] = cmd_ist;    /* insert assumed delimiter */
  cmd_buffer[n++] = ';';                         /* in case not present */
  cmd_buffer[n] = 0;
  cmd_ptr = cmd_buffer;
  return TRUE;
  }
}




/*************************************************
*             Confirm Output Wanted              *
*************************************************/

/* The yield is 0 for yes, 1 for no, 2 for stop, 3 for
discard, and 4 for a new file name. */

int cmd_confirmoutput(char *name, BOOL stopflag, BOOL discardflag, BOOL toflag,
  int buffno, char **aname)
{
char buff[256];
char *pname = name;
char *dots = "";
int yield = 0;
BOOL yesok = (name != NULL);

*aname = NULL;   /* initialize to no new name */

if (!main_interactive ||
  ((currentbuffer->noprompt || !main_warnings) && yesok))
    return 0;

/* Cope with overlong names */

if (yesok && (int)strlen(pname) > 100)
  {
  pname += strlen(pname) - 100;
  dots = "...";
  }

if (buffno >= 0)
  {
  if (yesok)
    sprintf(buff, "Write buffer %d to %s%s? (Y/N",
      buffno, dots, pname);
  else
    sprintf(buff, "Write buffer %d? (N", buffno);
  }
else
  {
  if (yesok)
    sprintf(buff, "Write to %s%s? (Y/N", dots, pname);
  else
    sprintf(buff, "Write? (N");
  }

sprintf(buff + strlen(buff), "%s%s%s) ", toflag? "/TO filename" : "",
  discardflag? "/Discard" : "", stopflag? "/STOP" : "");

/* Deal with overlong prompt line; can only happen if a file name
has been included. */

if (main_screenOK && (int)strlen(buff) > window_width)
  {
  int shortenby = (int)strlen(buff) - window_width;
  char *at = strchr(buff, 'o');
  if (at != NULL)    /* just in case */
    {
    memmove(at+2, at+2+shortenby, (int)strlen(buff) - ((at - buff) + 2 + shortenby) + 1);
    strncpy(at+2, "...", 3);
    }
  }

/* Loop during bad responses, ensuring errors get an immediate display
if windowing */

error_werr = TRUE;

for (;;)
  {
  BOOL done = FALSE;

  while (!done)
    {
    char *p = cmd_buffer;
    if (main_screenOK) scrn_rdline(FALSE, FALSE, buff); else
      {
      error_printf(buff);
      error_printflush();
      if (fgets(cmd_buffer, cmd_buffer_size, kbd_fid) == NULL) return 0;
      }
    if (cmd_buffer[strlen(cmd_buffer)-1] == '\n')
      cmd_buffer[strlen(cmd_buffer)-1] = 0;
    while (*p != 0)
      if (*p++ != ' ') { done = TRUE; break; }
    }

  cmd_ptr = cmd_buffer;
  mac_skipspaces(cmd_ptr);
  cmd_readword();
  mac_skipspaces(cmd_ptr);

  if (*cmd_ptr == 0 || *cmd_ptr == '\n')
    {
    if ((strcmp(cmd_word, "y") == 0 || strcmp(cmd_word, "yes") == 0) && yesok) break;
    if (strcmp(cmd_word, "n") == 0 || strcmp(cmd_word, "no") == 0)
      { yield = 1; break; }
    if (strcmp(cmd_word, "stop") == 0 && stopflag) { yield = 2; break; }
    if ((strcmp(cmd_word, "d") == 0 || strcmp(cmd_word, "discard") == 0) && discardflag)
      { yield = 3; break; }
    error_moan(11);
    }

  else if (!toflag || strcmp(cmd_word, "to") != 0) error_moan(toflag? 11:55); else
    {
    char *msg = sys_checkfilename(cmd_ptr, FALSE);
    if (msg == NULL)
      {
      *aname = cmd_ptr;
      yield = 4;
      break;
      }
    else error_moan(12, cmd_ptr, msg);
    }
  }

error_werr = FALSE;
return yield;
}




/*************************************************
*             Mark file changed                  *
*************************************************/

void cmd_recordchanged(linestr *line, int col)
{
main_filechanged = TRUE;
main_backprevptr = -1;
(main_backlist[main_backptr]).line = line;
(main_backlist[main_backptr]).col = col;
}


/*************************************************
*           Save back up position                *
*************************************************/

void cmd_saveback(void)
{
if ((main_backlist[main_backptr]).line != NULL)
  {
  if (++main_backptr >= back_size) main_backptr = 0;
  (main_backlist[main_backptr]).line = NULL;
  }
}


/*************************************************
*          Find a numbered buffer                *
*************************************************/

bufferstr *cmd_findbuffer(int n)
{
bufferstr *yield = main_bufferchain;
while (yield != NULL)
  {
  if (yield->bufferno == n) return yield;
  yield = yield->next;
  }
return NULL;
}



/*************************************************
*             Empty a buffer                     *
*************************************************/

/* Subroutine to empty a buffer, close any files that are
associated with it, and free any associated store. Called by
DBUFFER and LOAD. Note that we do not want to select the
buffer, as that would cause an unnecessary screen refresh.
The buffer block must be re-initialized before re-use. The
yield if FALSE if prompting gets a negative reply. */

BOOL cmd_emptybuffer(bufferstr *buffer, char *cmdname)
{
linestr *line;
FILE *ffid;
FILE *tfid = buffer->to_fid;
int linecount = buffer->linecount;
char *filealias = buffer->filealias;
char *filename = buffer->filename;

/* Ensure relevant cached values are filed back */

if (buffer == currentbuffer)
  {
  buffer->from_fid = from_fid; 
  buffer->changed = main_filechanged;
  buffer->top = main_top;
  buffer->bottom = main_bottom;
  }

ffid = buffer->from_fid;

if (buffer->changed && !buffer->noprompt && main_warnings)
  {
  #ifdef Windowing
  if (main_windowing)
    {
    char *name = buffer->filealias;
    if (name == NULL) name = "<no name>";
    if (!sys_window_yesno(FALSE, "Continue",
      "The contents of buffer %d (%s) have not been saved\n",
        buffer->bufferno, name)) return FALSE;
    }
  else
  #endif
    {
    error_moan(24, buffer->bufferno);
    if (!cmd_yesno("Continue with %s (Y/N)? ", cmdname)) return FALSE;
    }
  }

#ifdef Windowing
if (main_windowing) sys_hourglass(TRUE);
#endif

line = buffer->top;
while (line != NULL)
  {
  linestr *next = line->next;

  if (main_interrupted(ci_delete))
    {
    line->prev = NULL;
    buffer->top = buffer->current = line;
    buffer->linecount = linecount;
    buffer->col = 0;
    buffer->currentlinenumber = 1;

    if (buffer == currentbuffer)
      {
      main_linecount = linecount;
      main_current = main_top = line;
      main_currentlinenumber = 1;
      cursor_col = 0;
      }
    error_moan(57);

    #ifdef Windowing
    if (main_windowing) sys_hourglass(FALSE);
    #endif
    return FALSE;
    }

  if (line->text != NULL) store_free(line->text);
  store_free(line);
  linecount--;
  line = next;
  }

store_free(filealias);
store_free(filename);
store_free(buffer->backlist);

if (ffid != NULL) fclose(ffid);
if (tfid != NULL) fclose(tfid);

#ifdef Windowing
if (main_windowing) sys_hourglass(FALSE);
#endif

return TRUE;
}



/*************************************************
*            Simple yes/no prompt                *
*************************************************/

/* A yield of TRUE means 'yes', and this is always given
if running non-interactively. */

BOOL cmd_yesno(char *s, ...)
{
BOOL yield = TRUE;
va_list ap;
char prompt[256];

va_start(ap, s);

vsprintf(prompt, s, ap);

#ifdef Windowing
if (main_windowing) yield = sys_window_yesno(FALSE, "Continue", prompt); else
#endif

if (main_interactive) for (;;)
  {
  if (main_screenOK)
    {
    scrn_rdline(FALSE, FALSE, prompt);
    printf("\n");
    }
  else
    {
    error_printf(prompt);
    error_printflush();
    if (fgets(cmd_buffer, cmd_buffer_size, cmdin_fid) == NULL) break;
    }

  cmd_ptr = cmd_buffer;
  cmd_readword();

  if (cmd_atend())
    {
    if (strcmp(cmd_word, "y") == 0 || strcmp(cmd_word, "yes") == 0) break;
      else if (strcmp(cmd_word, "n") == 0 || strcmp(cmd_word, "no") == 0)
        { yield = FALSE; break; }
    }
  }

if (yield) main_pendnl = main_nowait = TRUE;
return yield;
}



/*************************************************
*           Read Procedure Name                  *
*************************************************/

BOOL cmd_readprocname(stringstr **aname)
{
char *p, *q;
int n;
stringstr *st;

mac_skipspaces(cmd_ptr);
p = cmd_ptr;

if (*cmd_ptr++ != '.')
  {
  error_moan(46);
  return FALSE;
  }

while (isalnum((unsigned int)(*cmd_ptr))) cmd_ptr++;
if (*cmd_ptr != 0 && *cmd_ptr != ' ' && *cmd_ptr != ';' && *cmd_ptr != ')')
  {
  error_moan(46);
  return FALSE;
  }

n = cmd_ptr - p;
st = store_Xget(sizeof(stringstr));
st->type = cb_sttype;
st->delim = ' ';
st->hexed = FALSE;
st->text = q = store_Xget(n + 1);

while (n--) *q++ = tolower(*p++);
*q = 0;

mac_skipspaces(cmd_ptr);
*aname = st;               /* pass back control block */
return TRUE;
}



/*************************************************
*              Find procedure                    *
*************************************************/

/* If found, move to top of list. The pointer is returned
via ap, unless it is NULL. */

BOOL cmd_findproc(char *name, procstr **ap)
{
procstr *p = main_proclist;
procstr *pp = NULL;
while (p != NULL)
  {
  if (strcmp(name, p->name) == 0)
    {
    if (ap != NULL) { *ap = p; }
    if (pp != NULL)
      {
      pp->next = p->next;
      p->next = main_proclist;
      main_proclist = p;
      }
    return TRUE;
    }
  pp = p;
  p = p->next;
  }
return FALSE;
}


/* End of ecmdsub.c */
