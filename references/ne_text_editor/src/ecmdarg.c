/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1999 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: December 1999 */


/* This file contains code for reading the arguments of commands */


#include "ehdr.h"
#include "cmdhdr.h"

static void c_iline(cmdstr *); /* Used before defined */
static void c_save(cmdstr *);  /* Used before defined */


/*************************************************
*            Command with no arguments           *
*************************************************/

static void noargs(cmdstr *c)
{
c = c;
}


/*************************************************
*     Align, Closeup, Dleft, Dline & Dright      *
*************************************************/

static void c_align(cmdstr *c)     { c->misc = lb_align; }
static void c_closeback(cmdstr *c) { c->misc = lb_closeback; }
static void c_closeup(cmdstr *c)   { c->misc = lb_closeup; }
static void c_dleft(cmdstr *c)     { c->misc = lb_eraseleft; }
static void c_dline(cmdstr *c)     { c->misc = lb_delete; }
static void c_dright(cmdstr *c)    { c->misc = lb_eraseright; }



/*************************************************
*         The xA, xB and xE commands             *
*************************************************/

/* This is used for GA, GE, GB, A, B, E */

static void c_abe(cmdstr *cmd, BOOL gflag, int misc)
{
cmd->misc = misc;

if (cmd_atend()) return;    /* no argument supplied */
match_L = FALSE;            /* for compiling regular expressions */

/* Check for prompted first arg, or read first arg */

if (cmd_readse(&cmd->arg1.se))
  {
  cmd->flags |= cmdf_arg1 + cmdf_arg1F;
  if (gflag && (cmd->arg1.se)->type == cb_qstype)
    {
    qsstr *q = cmd->arg1.qs;
    if (q->length == 0 && (q->flags & (qsef_B+qsef_E)) == 0)
      { error_moan(27); cmd_faildecode = TRUE; return; }
    }
  }
else { cmd_faildecode = TRUE; return; }


/* Check for prompted second arg, or read second arg */

if (cmd_readqualstr(&cmd->arg2.qs, rqs_XRonly))
  cmd->flags |= cmdf_arg2 + cmdf_arg2F;
else { cmd_faildecode = TRUE; return; }
}


static void c_ga(cmdstr *cmd) { c_abe(cmd, TRUE, abe_a); }
static void c_gb(cmdstr *cmd) { c_abe(cmd, TRUE, abe_b); }
static void c_ge(cmdstr *cmd) { c_abe(cmd, TRUE, abe_e); }

static void c_a(cmdstr *cmd) { c_abe(cmd, FALSE, abe_a); }
static void c_b(cmdstr *cmd) { c_abe(cmd, FALSE, abe_b); }
static void c_e(cmdstr *cmd) { c_abe(cmd, FALSE, abe_e); }



/*************************************************
*            The yA, yB  commands                *
*************************************************/

/* This is used for PA, PB, SA, SB, DTA, DTB */

static void c_ab(cmdstr *cmd, int misc)
{
cmd->misc = misc;
if (cmd_readse(&cmd->arg1.se)) cmd->flags |= cmdf_arg1 + cmdf_arg1F;
  else cmd_faildecode = TRUE;
}

static void c_pa(cmdstr *cmd) { c_ab(cmd, abe_a); }
static void c_pb(cmdstr *cmd) { c_ab(cmd, abe_b); }



/*************************************************
*            The AUTOALIGN command               *
*************************************************/

/* Also used by several other commands that have on/off arguments */

static void c_autoalign(cmdstr *cmd)
{
cmd_readword();
if (cmd_word[0] != 0)
  {
  cmd->flags |= cmdf_arg1;
  if (strcmp(cmd_word, "on") == 0) cmd->arg1.value = TRUE;
  else if (strcmp(cmd_word, "off") == 0) cmd->arg1.value = FALSE;
  else
    {
    error_moan(13, "\"on\" or \"off\"");
    cmd_faildecode = TRUE;
    }
  }
}



/*************************************************
*             The BACKUP command                 *
*************************************************/

/* Currently very restricted */

static void c_backup(cmdstr *cmd)
{
cmd_readword();
if (cmd_word[0] != 0 && strcmp(cmd_word, "files") == 0)
  {
  cmd->misc = backup_files;
  c_autoalign(cmd);
  }
else
  {
  error_moan(13, "\"files\"");
  cmd_faildecode = TRUE;
  }
}



/*************************************************
*            The BEGINPAR command                *
*************************************************/

/* Also used for endpar */

static void c_beginpar(cmdstr *cmd)
{
if (cmd_readse(&cmd->arg1.se)) cmd->flags |= cmdf_arg1 | cmdf_arg1F;
  else cmd_faildecode = TRUE;    /* message already given */
}



/*************************************************
*       The {C,CD,P}BUFFER commands              *
*************************************************/

static void allbuffer(cmdstr *cmd, int misc)
{
cmd->misc = misc;
cmd->arg1.value = cmd_readnumber();
if (cmd->arg1.value >= 0) cmd->flags |= cmdf_arg1;
}

static void c_buffer(cmdstr *cmd)   { allbuffer(cmd, FALSE); }
static void c_pbuffer(cmdstr *cmd)  { allbuffer(cmd, TRUE); }
static void c_cbuffer(cmdstr *cmd)  { allbuffer(cmd, cbuffer_c); }
static void c_cdbuffer(cmdstr *cmd) { allbuffer(cmd, cbuffer_cd); }




/*************************************************
*              The BREAK command                 *
*************************************************/

static void c_break(cmdstr *cmd)
{
cmd->arg1.value = cmd_readnumber();
if (cmd->arg1.value >= 0) cmd->flags |= cmdf_arg1;
}



/*************************************************
*             The COMMENT command                *
*************************************************/

static void c_comment(cmdstr *cmd)
{
int rc = cmd_readstring(&cmd->arg1.string);
if (rc <= 0)
  {
  if (rc == 0) error_moan(13, "string");
  cmd_faildecode = TRUE;
  }
else cmd->flags |= cmdf_arg1;
}


/*************************************************
*            The CL command                      *
*************************************************/

static void c_cl(cmdstr *cmd) { if (!cmd_atend()) c_iline(cmd); }



/*************************************************
*            The CPROC command                   *
*************************************************/

static void c_cproc(cmdstr *cmd)
{
if (!cmd_readprocname(&(cmd->arg1.string))) cmd_faildecode = TRUE;
  else cmd->flags |= cmdf_arg1;
}



/*************************************************
*             The CUTSTYLE command               *
*************************************************/

/* Also used by several other commands that have on/off arguments */

static void c_cutstyle(cmdstr *cmd)
{
cmd_readword();
if (cmd_word[0] != 0)
  {
  cmd->flags |= cmdf_arg1;
  if (strcmp(cmd_word, "append") == 0) cmd->arg1.value = TRUE;
  else if (strcmp(cmd_word, "replace") == 0) cmd->arg1.value = FALSE;
  else
    {
    error_moan(13, "\"append\" or \"replace\"");
    cmd_faildecode = TRUE;
    }
  }
}


/*************************************************
*           The DEBUG command                    *
*************************************************/

static void c_debug(cmdstr *cmd)
{
cmd_readword();
if (strcmp(cmd_word, "crash") == 0)
  {
  cmd->arg1.value = debug_crash;
  cmd->flags |= cmdf_arg1;
  }
else if (strcmp(cmd_word, "exceedstore") == 0)
  {
  cmd->arg1.value = debug_exceedstore;
  cmd->flags |= cmdf_arg1;
  }
if (strcmp(cmd_word, "nullline") == 0)
  {
  cmd->arg1.value = debug_nullline;
  cmd->flags |= cmdf_arg1;
  }
if (strcmp(cmd_word, "baderror") == 0)
  {
  cmd->arg1.value = debug_baderror;
  cmd->flags |= cmdf_arg1;
  }
}



/*************************************************
*               The DETRAIL command              *
*************************************************/

static void c_detrail(cmdstr *cmd)
{
cmd_readword();
if (cmd_word[0])
  {
  if (strcmp(cmd_word, "output") == 0) cmd->misc = detrail_output;
    else error_moan(13, "\"output\"");
  }
else cmd->misc = detrail_buffer;
}




/*************************************************
*               The F command                    *
*************************************************/

static void c_f_bf(cmdstr *cmd)
{
if (!cmd_atend())
  {
  if (cmd_readse(&cmd->arg1.se))
    {
    cmd->flags |= cmdf_arg1 + cmdf_arg1F;
    }
  else cmd_faildecode = TRUE;    /* message already given */
  }
}

/* We must set match_L for compiling regular expressions */

static void c_bf(cmdstr *cmd) { match_L = TRUE;  c_f_bf(cmd); }
static void c_f(cmdstr *cmd)  { match_L = FALSE; c_f_bf(cmd); }



/*************************************************
*            The FKEYSTRING command              *
*************************************************/

static void c_fks(cmdstr *cmd)
{
int n = cmd_readnumber();
if (1 <= n  && n <= max_keystring)
  {
  int rc = cmd_readstring(&cmd->arg2.string);
  if (rc < 0) cmd_faildecode = TRUE; else
    {
    cmd->arg1.value = n;
    cmd->flags |= cmdf_arg1;
    if (rc) cmd->flags |= cmdf_arg2;
    }
  }
else
  {
  if (n < 0) error_moan(13, "Number"); else error_moan(35, max_keystring);
  cmd_faildecode = TRUE;
  }
}


/*************************************************
*              The HELP command                  *
*************************************************/

static void c_help(cmdstr *cmd)
{
if (cmd_readrestofline(&cmd->arg1.string) > 0) cmd->flags |= cmdf_arg1;
}


/*************************************************
*             The IF command                     *
*************************************************/

/* First subroutine used by IF, UNLESS, WHILE, UNTIL */

static void ifulwhut(cmdstr *cmd, int misc, void (*proc)(cmdstr *, int))
{
char *savecmdptr = cmd_ptr;
match_L = FALSE;    /* for compiling regular expression */

/* Check for "eof", etc., "mark", "prompt" or read a search expression */

cmd_readword();

if (strcmp(cmd_word, "mark") == 0) misc |= if_mark;
else if (strcmp(cmd_word, "eol") == 0) misc |= if_eol;
else if (strcmp(cmd_word, "sol") == 0) misc |= if_sol;
else if (strcmp(cmd_word, "sof") == 0) misc |= if_sof;
else if (strcmp(cmd_word, "prompt") == 0)
  {
  misc |= if_prompt;
  if (cmd_readstring(&cmd->arg1.string) <= 0)
    {
    cmd_faildecode = TRUE;
    return;
    }
  cmd->flags |= cmdf_arg1 | cmdf_arg1F;
  }
else if (strcmp(cmd_word, "eof") != 0)
  {
  cmd_ptr = savecmdptr;
  if (!cmd_readse(&cmd->arg1.se))
    { cmd_faildecode = TRUE; return; }
  cmd->flags |= cmdf_arg1 | cmdf_arg1F;
  }

/* Insist on "THEN" or "DO" */

cmd_readword();
if (strcmp(cmd_word, "then") != 0 && strcmp(cmd_word, "do") != 0)
  {
  error_moan(13, "\"then\" or \"do\"");
  cmd_faildecode = TRUE;
  return;
  }

/* Call given proc to read one or two command sequence args */

proc(cmd, misc);
}


/* Subroutine used by IF and UNLESS to read two alternative
command groups. They are packed up into a single argument. */

static void ifularg2(cmdstr *cmd, int misc)
{
char *saveptr = NULL;
ifstr *arg = store_Xget(sizeof(ifstr));

cmd->arg2.ifelse = arg;
cmd->flags |= cmdf_arg2 | cmdf_arg2F;
cmd->misc = misc;

arg->type = cb_iftype;
arg->if_then = cmd_compile();
arg->if_else = NULL;
if (cmd_faildecode) return;

/* We must now cope with optional else clauses. People forget
whether a semicolon is or is not allowed, so we are forgiving about
this, since there is no ambguity. Inside brackets, or
non-interactively, we allow continuation lines for else, provided
the first line does not end in a closing bracket. If we don't find
else, we must terminate the if and restore the pointer. On the other
hand, if we don't find else on a single line, moan. */

/* First skip over semicolons, setting the restart at the last
one (since a sequence of them is the same as one). */

mac_skipspaces(cmd_ptr);
while (*cmd_ptr == ';')
  {
  saveptr = cmd_ptr++;
  while (*cmd_ptr == ' ') cmd_ptr++;
  }

/* Deal with unbracketted, interactive case */

if (cmd_bracount <= 0 && main_interactive && cmdin_fid == NULL)
  {
  if (cmd_atend() || *cmd_ptr == '\\') return;
  }
else

/* Deal with the bracketted case -- may have to join many lines until
we find one that does not consist solely of spaces or semicolons. */

for (;;)
  {
  if (*cmd_ptr == ')') return;
  if (cmd_atend() || *cmd_ptr == '\\')
    {
    if (!cmd_joinline(TRUE)) return;    /* return if eof */
    saveptr = cmd_ptr;                  /* the joining semicolon */
    while (*cmd_ptr == ';' || *cmd_ptr == ' ') cmd_ptr++;
    }
  else break;
  }

/* We now have something else to read, either on the original
unbracketted line, or on a concatenated line. If it is else we
read commands; if not, we reset the pointer for reading a new
command if a semicolon or end of line terminated the THEN part.
Otherwise we complain. */

cmd_readword();
if (strcmp(cmd_word, "else") == 0) arg->if_else = cmd_compile(); else
  {
  if (saveptr != NULL) { cmd_ptr = saveptr; return; }
  error_moan(13, "else");
  cmd_faildecode = TRUE;
  }
}


/* The calling point for IF */

static void c_if(cmdstr *cmd)
{
ifulwhut(cmd, if_if, ifularg2);
}



/*************************************************
*              The ILINE command                 *
*************************************************/

static void c_iline(cmdstr *cmd)
{
if (cmd_readqualstr(&cmd->arg1.qs, rqs_Xonly))
  {
  cmd->flags |= cmdf_arg1 | cmdf_arg1F;
  }
else cmd_faildecode = TRUE;
}



/*************************************************
*            The KEY command                     *
*************************************************/

static void c_key(cmdstr *cmd)
{
int rc = cmd_readUstring(&cmd->arg1.string);

if (rc < 0) cmd_faildecode = TRUE; else if (rc == 0)
  {
  error_moan(13, "Key definition(s)");
  cmd_faildecode = TRUE;
  }
else
  {
  cmd->flags |= cmdf_arg1;
  cmd_faildecode = !key_set(cmd->arg1.string->text, FALSE);  /* syntax check */
  }
}



/*************************************************
*                The M command                   *
*************************************************/

static void c_m(cmdstr *cmd)
{
cmd->arg1.value = cmd_readnumber();
if (cmd->arg1.value < 0)
  {
  if (*cmd_ptr == '*') cmd_ptr++; else
    {
    error_moan(13, "Number or \"*\"");
    cmd_faildecode = TRUE;
    return;
    }
  }
cmd->flags |= cmdf_arg1;
}


/*************************************************
*           The MAKEBUFFER command               *
*************************************************/

static void c_makebuffer(cmdstr *cmd)
{
cmd->arg2.value = cmd_readnumber();
if (cmd->arg2.value < 0)
  {
  error_moan(13, "Number");
  cmd_faildecode = TRUE;
  return;
  }
cmd->flags |= cmdf_arg2;
c_save(cmd);
}



/*************************************************
*           The MARK command                     *
*************************************************/

static void c_mark(cmdstr *cmd)
{
cmd_readword();
if (strcmp(cmd_word, "limit") == 0) cmd->misc = amark_limit;
else if (strcmp(cmd_word, "line") == 0 || strcmp(cmd_word, "lines") == 0)
  {
  if (cmd_atend()) cmd->misc = amark_line; else
    {
    cmd_readword();
    if (strcmp(cmd_word, "hold") == 0) cmd->misc = amark_hold; else
      {
      error_moan(13, "hold");
      cmd_faildecode = TRUE;
      }
    }
  }
else if (strcmp(cmd_word,"rectangle") == 0)
  cmd->misc = amark_rectangle;
else if (strcmp(cmd_word,"text") == 0)
  cmd->misc = amark_text;
else if (strcmp(cmd_word,"unset") == 0)
  cmd->misc = amark_unset;
else
  {
  error_moan(13, "\"limit\", \"line\", \"rectangle\", \"text\" or \"unset\"");
  cmd_faildecode = TRUE;
  }
}




/*************************************************
*           The NAME command                     *
*************************************************/


static void readfilename(cmdstr *cmd, BOOL checkflag)
{
if (cmd_atend())
  {
  error_moan(13, "File name");
  cmd_faildecode = TRUE;
  }
else
  {
  int rc = ((ch_tab[(unsigned int)(*cmd_ptr)] & ch_filedelim) == 0)?
    cmd_readUstring(&cmd->arg1.string) : cmd_readstring(&cmd->arg1.string);

  if (rc < 0) cmd_faildecode = TRUE; else    /* syntax error */
    {
    char *rcm = checkflag? sys_checkfilename((cmd->arg1.string)->text, FALSE) : NULL;
    cmd->flags |= cmdf_arg1 + cmdf_arg1F;
    if (rcm != NULL)
      {
      error_moan(12, (cmd->arg1.string)->text, rcm);
      cmd_faildecode = TRUE;
      }
    }
  }
}

static void c_name(cmdstr *cmd)  { readfilename(cmd, TRUE); }    /* with sys check */
static void c_namex(cmdstr *cmd) { readfilename(cmd, FALSE); }   /* without ditto */


/*************************************************
*            The PLL & PLR commands              *
*************************************************/

static void c_pll(cmdstr *cmd) { cmd->misc = abe_b; }
static void c_plr(cmdstr *cmd) { cmd->misc = abe_a; }





/*************************************************
*            The PROC command                    *
*************************************************/

static void c_proc(cmdstr *cmd)
{
if (!cmd_readprocname(&(cmd->arg1.string)))
  { cmd_faildecode = TRUE; return; }

cmd->flags |= cmdf_arg1;
cmd_readword();
if (strcmp(cmd_word, "is") == 0)
  {
  cmdstr *body = cmd_compile();
  if (cmd_faildecode) return;
  cmd->arg2.cmds = body;
  if (body != NULL) cmd->flags |= cmdf_arg2;
  }
else
  {
  cmd_faildecode = TRUE;
  error_moan(13, "\"is\"");
  }
}



/*************************************************
*            The REPEAT command                  *
*************************************************/

static void c_repeat(cmdstr *cmd)
{
cmd->arg1.cmds = cmd_compile();
cmd->flags |= cmdf_arg1 | cmdf_arg1F;
}



/*************************************************
*           The RMARGIN command                  *
*************************************************/

static void c_rmargin(cmdstr *cmd)
{
cmd_readword();
if (cmd_word[0] != 0)
  {
  cmd->flags |= cmdf_arg2;
  if (strcmp(cmd_word, "on") == 0) cmd->arg2.value = TRUE;
  else if (strcmp(cmd_word, "off") == 0) cmd->arg2.value = FALSE;
  else
    {
    error_moan(13, "\"on\" or \"off\" or a number");
    cmd_faildecode = TRUE;
    }
  }
else
  {
  cmd->arg1.value = cmd_readnumber();
  if (cmd->arg1.value >= 0) cmd->flags |= cmdf_arg1;
  }
}



/*************************************************
*             The SAVE command                   *
*************************************************/

static void c_save(cmdstr *cmd)
{
if (!cmd_atend()) c_name(cmd);
}


/*************************************************
*              The SET command                   *
*************************************************/

static void c_set(cmdstr *cmd)
{
cmd_readword();
if (strcmp(cmd_word, "autovscroll") == 0)
  {
  int n = cmd_readnumber();
  cmd->misc = set_autovscroll;

  if (!main_screenOK || (1 <= n  && n <= window_depth-2))
    {
    cmd->arg1.value = n;
    cmd->flags |= cmdf_arg1;
    }
  else
    {
    error_moan(34, "autovscroll", "not in range 1 to (screen depth - 2)");
    cmd_faildecode = TRUE;
    }
  }

else if (strcmp(cmd_word, "splitscrollrow") == 0)
  {
  int n = cmd_readnumber();
  if (n > 0)
    {
    cmd->misc = set_splitscrollrow;
    cmd->arg1.value = n - 1;
    cmd->flags |= cmdf_arg1;
    }
  else
    {
    error_moan(13, "Positive number");
    cmd_faildecode = TRUE;
    }
  }

else if (strcmp(cmd_word, "oldcommentstyle") == 0)
  {
  cmd->misc = set_oldcommentstyle;
  }

else if (strcmp(cmd_word, "newcommentstyle") == 0)
  {
  cmd->misc = set_oldcommentstyle;
  }

else   /* unknown SET option */
  {
  error_moan(13, "\"autovscroll\" or \"splitscrollrow\"");
  cmd_faildecode = TRUE;
  }
}



/*************************************************
*              The SHOW command                  *
*************************************************/

static void c_show(cmdstr *cmd)
{
cmd_readword();
if (strcmp(cmd_word, "ckeys") == 0)           cmd->misc = show_ckeys;
else if (strcmp(cmd_word, "fkeys") == 0)      cmd->misc = show_fkeys;
else if (strcmp(cmd_word, "xkeys") == 0)      cmd->misc = show_xkeys;
else if (strcmp(cmd_word, "keys") == 0)       cmd->misc = show_allkeys;
else if (strcmp(cmd_word, "keystrings") == 0) cmd->misc = show_keystrings;
else if (strcmp(cmd_word, "buffers") == 0)    cmd->misc = show_buffers;
else if (strcmp(cmd_word, "wordcount") == 0)  cmd->misc = show_wordcount;
else if (strcmp(cmd_word, "version") == 0)    cmd->misc = show_version;
else if (strcmp(cmd_word, "keyactions") == 0) cmd->misc = show_actions;
else if (strcmp(cmd_word, "commands") == 0)   cmd->misc = show_commands;
else if (strcmp(cmd_word, "wordchars") == 0)  cmd->misc = show_wordchars;
else if (strcmp(cmd_word, "settings") == 0)   cmd->misc = show_settings;
else
  {
  error_moan(13, "keys, ckeys, fkeys, xkeys, keystrings, buffers, keyactions, "
    "commands,\n   wordcount, settings, or version");
  cmd_faildecode = TRUE;
  }
}


/*************************************************
*           The STREAMMAX command                *
*************************************************/

static void c_streammax(cmdstr *cmd)
{
cmd->arg1.value = cmd_readnumber();
if (cmd->arg1.value < 0)
  {
  error_moan(13, "Number");
  cmd_faildecode = TRUE;
  return;
  }
  if (cmd->arg1.value < 500) cmd->arg1.value = 500;
cmd->flags |= cmdf_arg1;
}



/*************************************************
*             The TL command                     *
*************************************************/

static void c_tl(cmdstr *cmd)
{
cmd->misc = TRUE;
c_m(cmd);
}


/*************************************************
*             The UNLESS command                 *
*************************************************/

static void c_unless(cmdstr *cmd)
{
ifulwhut(cmd, if_unless, ifularg2);
}


/*************************************************
*         The UNTIL & UTEOF commands             *
*************************************************/

/* Subroutine used by UNTIL && WHILE to read second argument */

static void utwharg2(cmdstr *cmd, int misc)
{
cmd->misc = misc;
cmd->arg2.cmds = cmd_compile();
cmd->flags |= cmdf_arg2 | cmdf_arg2F;

if (cmd->arg2.cmds == NULL)    /* lock out null second arguments */
  {
  error_moan(33, (misc == if_unless)? "until" : "while");
  cmd_faildecode = TRUE;
  }
}


static void c_until(cmdstr *cmd)
{
ifulwhut(cmd, if_unless, utwharg2);
}

static void c_uteof(cmdstr *cmd)
{
utwharg2(cmd, if_unless);
}



/*************************************************
*               The WHILE command                *
*************************************************/

static void c_while(cmdstr *cmd)
{
ifulwhut(cmd, if_if, utwharg2);
}



/*************************************************
*              The WORD command                  *
*************************************************/

static void c_word(cmdstr *cmd)
{
char *p;
char *e = NULL;
int rc = cmd_readstring(&cmd->arg1.string);

if (rc <= 0)
  {
  if (rc == 0) error_moan(13, "String");
  cmd_faildecode = TRUE;
  return;
  }

cmd->flags |= cmdf_arg1 | cmdf_arg1F;
p = cmd->arg1.string->text;

while (*p)
  {
  int a = *p++;

  if (a == '\"')
    {
    if ((a = *p++) == 0) e = "unexpected end";
    }

  if (*p == '-')
    {
    int b;
    if ((b = *(++p)) == 0) e = "unexpected end"; else
      {
      int ta = ch_tab[a] & (ch_letter+ch_digit);
      int tb = ch_tab[b] & (ch_letter+ch_digit);
      p++;
      if (ta == 0 || ta != tb)
        {
        e = "\n   only digits or letters of the same case are allowed in a range";
        break;
        }
      if (b < a)
        {
        e = "characters out of order in a range";
        break;
        }
      }
    }
  }

if (e != NULL)
  {
  error_moan(44, p - cmd->arg1.string->text, e);
  cmd_faildecode = TRUE;
  }
}



/*************************************************
*            Table of arg functions              *
*************************************************/

cmd_Cproc cmd_Cproclist[] = {
  c_a,          /* a */
  noargs,       /* abandon */
  c_align,      /* align */
  c_autoalign,  /* attn */
  c_autoalign,  /* autoalign */
  c_b,          /* b */
  noargs,       /* back */
  c_backup,     /* backup */
  c_beginpar,   /* beginpar */
  c_bf,         /* bf */
  c_break,      /* break */
  c_buffer,     /* buffer */
  c_name,       /* c */
  c_autoalign,  /* casematch */
  c_cbuffer,    /* cbuffer */
  c_cdbuffer,   /* cdbuffer */
  noargs,       /* center */
  noargs,       /* centre */
  c_cl,         /* cl */
  c_closeback,  /* closeback */
  c_closeup,    /* closeup */
  c_comment,    /* comment */
  noargs,       /* copy */
  c_cproc,      /* cproc */
  noargs,       /* csd */
  noargs,       /* csu */
  noargs,       /* cut */
  c_cutstyle,   /* cutstyle */
  c_buffer,     /* dbuffer */
  noargs,       /* dcut */
  c_debug,      /* debug */
  c_detrail,    /* detrail */
  c_f,          /* df */
  c_dleft,      /* dleft */
  c_dline,      /* dline */
  noargs,       /* dmarked */
  noargs,       /* drest */
  c_dright,     /* dright */
  c_pa,         /* dta */
  c_pb,         /* dtb */
  noargs,       /* dtwl */
  noargs,       /* dtwr */
  c_e,          /* e */
  c_autoalign,  /* eightbit */
  c_beginpar,   /* endpar */
  c_f,          /* f */
  c_fks,        /* fkeystring */
  c_fks,        /* fks */
  noargs,       /* format */
  c_ga,         /* ga */
  c_gb,         /* gb */
  c_ge,         /* ge */
  c_help,       /* help */
  c_save,       /* i */
  noargs,       /* icurrent */
  c_if,         /* if */
  c_iline,      /* iline */
  noargs,       /* ispace */
  c_key,        /* key  */
  noargs,       /* lcl */
  c_name,       /* load */
  c_break,      /* loop */
  c_m,          /* m */
  c_makebuffer, /* makebuffer */
  c_mark,       /* mark */
  noargs,       /* n */
  c_name,       /* name */
  c_save,       /* ne */
  c_save,       /* newbuffer */
  c_autoalign,  /* overstrike */
  noargs,       /* p */
  c_pa,         /* pa */
  c_buffer,     /* paste */
  c_pb,         /* pb */
  c_pbuffer,    /* pbuffer */
  c_pll,        /* pll */
  c_plr,        /* plr */
  c_proc,       /* proc */
  c_autoalign,  /* prompt */
  c_autoalign,  /* readonly */
  noargs,       /* refresh */
  noargs,       /* renumber */
  c_repeat,     /* repeat */
  c_rmargin,    /* rmargin */
  c_pa,         /* sa */
  c_save,       /* save */
  c_pb,         /* sb */
  c_set,        /* set */
  c_show,       /* show */
  noargs,       /* stop */
  c_name,       /* stream */
  c_streammax,  /* streammac */
  c_m,          /* t */
  c_namex,      /* title */
  c_tl,         /* tl */
  noargs,       /* topline */
  noargs,       /* ucl */
  noargs,       /* undelete */
  c_autoalign,  /* unixregexp */
  c_unless,     /* unless */
  c_until,      /* until */
  c_uteof,      /* ufeof */
  c_autoalign,  /* verify */
  noargs,       /* w    */
  c_autoalign,  /* warn */
  c_while,      /* while */
  c_word,       /* word */
  c_name        /* write */
};



/*************************************************
*            Table of execution functions        *
*************************************************/

/* We put this here for convenience of editing, so that
the two tables can be changed together. Note that the
table of command names is in ecmdcomp, and must be kept
in step too. */

cmd_Eproc cmd_Eproclist[] = {
  e_abe,        /* a */
  e_abandon,    /* abandon */
  e_actongroup, /* align */
  e_attn,       /* attn */
  e_autoalign,  /* autoalign */
  e_abe,        /* b */
  e_back,       /* back */
  e_backup,     /* backup */
  e_beginpar,   /* beginpar */
  e_bf,         /* bf */
  e_break,      /* break */
  e_buffer,     /* buffer */
  e_c,          /* c */
  e_casematch,  /* casematch */
  e_cdbuffer,   /* cbuffer (sic) */
  e_cdbuffer,   /* cdbuffer */
  e_centre,     /* center */
  e_centre,     /* centre */
  e_cl,         /* cl */
  e_actongroup, /* closeback */
  e_actongroup, /* closeup */
  e_comment,    /* comment */
  e_copy,       /* copy */
  e_cproc,      /* cproc */
  e_csd,        /* csd */
  e_csu,        /* csu */
  e_cut,        /* cut */
  e_cutstyle,   /* cutstyle */
  e_dbuffer,    /* dbuffer */
  e_dcut,       /* dcut */
  e_debug,      /* debug */
  e_detrail,    /* detrail */
  e_df,         /* df */
  e_actongroup, /* dleft */
  e_actongroup, /* dline */
  e_dmarked,    /* dmarked */
  e_drest,      /* drest */
  e_actongroup, /* dright */
  e_dtab,       /* dta */
  e_dtab,       /* dtb */
  e_dtwl,       /* dtwl */
  e_dtwr,       /* dtwr */
  e_abe,        /* e */
  e_eightbit,   /* eightbit */
  e_endpar,     /* endpar */
  e_f,          /* f */
  e_fks,        /* fkeystring */
  e_fks,        /* fks */
  e_format,     /* format */
  e_g,          /* ga */
  e_g,          /* gb */
  e_g,          /* ge */
  e_help,       /* help */
  e_i,          /* i */
  e_icurrent,   /* icurrent */
  e_if,         /* if */
  e_iline,      /* iline */
  e_ispace,     /* ispace */
  e_key,        /* key  */
  e_lcl,        /* lcl */
  e_load,       /* load */
  e_loop,       /* loop */
  e_m,          /* m */
  e_makebuffer, /* makebuffer */
  e_mark,       /* mark */
  e_n,          /* n */
  e_name,       /* name */
  e_newbuffer,  /* ne */
  e_newbuffer,  /* newbuffer */
  e_overstrike, /* overstrike */
  e_p,          /* p */
  e_pab,        /* pa */
  e_paste,      /* paste */
  e_pab,        /* pb */
  e_buffer,     /* pbuffer */
  e_plllr,      /* pll */
  e_plllr,      /* plr */
  e_proc,       /* proc */
  e_prompt,     /* prompt */
  e_readonly,   /* readonly */
  e_refresh,    /* refresh */
  e_renumber,   /* renumber */
  e_repeat,     /* repeat */
  e_rmargin,    /* rmargin */
  e_sab,        /* sa */
  e_save,       /* save */
  e_sab,        /* sb */
  e_set,        /* set */
  e_show,       /* show */
  e_stop,       /* stop */
  e_stream,     /* stream */
  e_streammax,  /* streammax */
  e_ttl,        /* t */
  e_title,      /* title */
  e_ttl,        /* tl */
  e_topline,    /* topline */
  e_ucl,        /* ucl */
  e_undelete,   /* undelete */
  e_unixregexp, /* unixregexp */
  e_if,         /* unless */
  e_while,      /* until */
  e_while,      /* uteof */
  e_verify,     /* verify */
  e_w,          /* w    */
  e_warn,       /* warn */
  e_while,      /* while */
  e_word,       /* word */
  e_write,      /* write */

  /* Special commands follow on directly */

  e_star,       /* *command */
  e_query,      /* ? */
  e_right,      /* > */
  e_left,       /* < */
  e_sharp,      /* # */
  e_dollar,     /* $ */
  e_percent,    /* % */
  e_tilde,      /* ~ */

  /* Bracketed group and procedure commands follow at the end */

  e_sequence,   /* sequence in brackets */
  e_obeyproc    /* procedure call */
};


/* End of ecmdarg.c */
