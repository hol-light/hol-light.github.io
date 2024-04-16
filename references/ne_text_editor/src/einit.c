/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1993, 1994 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: November 1994 */


/* This file contains initializing code, including the
main program, which is the entry point to E. It also contains
termination code. */


#include "ehdr.h"
#include "cmdhdr.h"


static char arg_from_buffer[100];
static char arg_to_buffer[100];

static jmp_buf rdline_env;



/*************************************************
*            Interrupt handler                   *
*************************************************/

/* We simply set a flag. Resetting the trap happens later
when the signal is noticed. This allows two interrupts to
kill a looping E. */

static void sigint_handler(int sig)
{
sig = sig;
main_escape_pressed = TRUE;
}



/*************************************************
*        Interrupt handler during fgets          *
*************************************************/

static void fgets_sigint_handler(int sig)
{
sig = sig;
main_escape_pressed = TRUE;
longjmp(rdline_env, 1);
}



/*************************************************
*           Check for interruption               *
*************************************************/

/* The call to sys_checkinterrupt() allows for checking in addition
to the normal signal handler. Used in Unix to check for ctrl/c
explicitly in raw input mode. To help avoid obeying expensive
system calls too often, a count of calls since the last command
line was read is maintained, and the type of activity in progress
at the time of the call is also passed on. */

BOOL main_interrupted(int type)
{
main_cicount++;
sys_checkinterrupt(type);
if (main_escape_pressed)
  {
  main_escape_pressed = FALSE;
  signal(SIGINT, sigint_handler);
  if (main_attn || main_oneattn)
    {
    main_oneattn = FALSE;
    error_moan(23);
    return TRUE;
    }
  else
    {
    main_oneattn = TRUE;
    return FALSE;
    }
  }
else return FALSE;
}


/*************************************************
*               Flush interruption               *
*************************************************/

void main_flush_interrupt(void)
{
if (main_escape_pressed)
  {
  main_escape_pressed = FALSE;
  signal(SIGINT, sigint_handler);
  }
}



/*************************************************
*         Check size of stream buffer            *
*************************************************/

/* This should be called only when currentbuffer->to_fid is non-NULL,
i.e. when the buffer is in stream mode and the output file has already
been opened. */

void main_checkstream(void)
{
while (main_linecount > currentbuffer->streammax)
  {
  linestr *line = main_top;
  int rc = file_writeline(line, currentbuffer->to_fid);

  if (rc < 0)
    {
    error_moan(37, currentbuffer->filealias, strerror(errno));
    return;
    }

  if (main_current == line)
    {
    main_current = line->next;
    cursor_col = 0;
    }

  line_delete(line, FALSE);
  cmd_refresh = TRUE;
  }
}


/*************************************************
*            Initialize a new buffer             *
*************************************************/

void init_buffer(bufferstr *buffer, int n, char *name, char *alias,
  FILE *ffid, FILE *tfid, int rmargin)
{
int i;
for (i = 0; i < sizeof(bufferstr)/sizeof(int); i++) ((int *)buffer)[i] = 0;
buffer->binoffset = 0;
buffer->bufferno = n;
buffer->changed = buffer->saved = FALSE;
buffer->imax = 1;
buffer->imin = 0;
buffer->rmargin = rmargin;
buffer->filename = name;
buffer->filealias = alias;
buffer->readonly = main_readonly;

buffer->backlist = store_Xget(back_size * sizeof(backstr));
for (i = 0; i < back_size; i++) ((buffer->backlist)[i]).line = NULL;
buffer->backptr = 0;
buffer->backprevptr = -1;

/* Set up first line in the buffer */

if (ffid == NULL)
  {
  buffer->top = store_getlbuff(0);
  buffer->top->flags |= lf_eof;
  }
else
  {
  buffer->top = file_nextline(&ffid, &buffer->binoffset, NULL);
  }

buffer->from_fid = ffid;
buffer->to_fid = tfid;

buffer->top->key = buffer->linecount = 1;
buffer->currentlinenumber = 0;
buffer->current = buffer->bottom = buffer->top;
}



/************************************************
*          Select Editing Buffer                *
************************************************/

void init_selectbuffer(bufferstr *buffer, BOOL changeflag)
{

/* First of all, salt away current parameters */

if (currentbuffer != NULL)
  {
  currentbuffer->backlist = main_backlist;
  currentbuffer->backprevptr = main_backprevptr;
  currentbuffer->backptr = main_backptr;
  currentbuffer->changed = main_filechanged;
  currentbuffer->currentlinenumber = main_currentlinenumber;
  currentbuffer->from_fid = from_fid;
  currentbuffer->imax = main_imax;
  currentbuffer->imin = main_imin;
  currentbuffer->linecount = main_linecount;
  currentbuffer->readonly = main_readonly;
  currentbuffer->row = cursor_row;
  currentbuffer->col = cursor_col;
  currentbuffer->offset = cursor_offset;
  currentbuffer->rmargin = main_rmargin;
  currentbuffer->top = main_top;
  currentbuffer->bottom = main_bottom;
  currentbuffer->current = main_current;
  currentbuffer->filename = main_filename;
  currentbuffer->filealias = main_filealias;

  currentbuffer->marktype = mark_type;
  currentbuffer->markline = mark_line;
  currentbuffer->markcol = mark_col;

  if (main_screenOK && !changeflag)
    currentbuffer->scrntop = window_vector[0];
  }

/* Now set parameters from saved block */

main_backlist = buffer->backlist;
main_backprevptr = buffer->backprevptr;
main_backptr = buffer->backptr;
main_filechanged = buffer->changed;
main_currentlinenumber = buffer->currentlinenumber;
from_fid = buffer->from_fid;
main_imax = buffer->imax;
main_imin = buffer->imin;
main_linecount = buffer->linecount;
cursor_row = buffer->row;
cursor_col = buffer->col;
cursor_offset = buffer->offset;
main_readonly = buffer->readonly;
main_rmargin = buffer->rmargin;
main_top = buffer->top;
main_bottom = buffer->bottom;
main_current = buffer->current;
main_filename = buffer->filename;
main_filealias = buffer->filealias;

mark_type = buffer->marktype;
mark_line = buffer->markline;
mark_col = buffer->markcol;


cursor_max = cursor_offset + window_width;
currentbuffer = buffer;

if (main_screenOK)
  {
  screen_forcecls = TRUE;
  scrn_hint(sh_topline, 0, buffer->scrntop);
  }
}





/*************************************************
*               Start up a file                  *
*************************************************/

/* This function is called when we start up the first window from
the quiescent state in a windowing environment, or at the start of
a non-windowing run. It expects any previous stuff to have been
tidied away already. In some modes of operation, the input file
has already been opened (to test for its existence). Hence the
alternative interfaces: either the fid and filetype can be passed,
or the name can be NULL (with a given filetype) or a name can be
passed, in which case the system must give a file type if relevant. */

BOOL init_init(FILE *fid, char *fromname, int filetype, char *toname, int stream)
{
FILE *to_fid = NULL;

if (fid == NULL && fromname != NULL && fromname[0] != 0)
  {
  if (strcmp(fromname, "-") == 0)
    {
    from_fid = stdin;
    main_interactive = main_verify = FALSE;
    if (cmdin_fid == stdin) cmdin_fid = NULL;
    if (msgs_fid == stdout) msgs_fid = stderr;
    }
  else
    {
    from_fid = sys_fopen(fromname, "r", &filetype);
    if (from_fid == NULL)
      {
      error_moan(5, fromname);
      return FALSE;
      }
    }
  }
else from_fid = fid;

/* If the flag main_stream is set, we are initializing for a non-windowing
run in stream mode, in which we have to hold the output open all the time.
The fact of stream mode is indicated by a non-NULL to_fid field in the buffer. */

if (stream != 0)
  {
  to_fid = sys_fopen(toname, "w", NULL);
  if (to_fid == NULL)
    {
    error_moan(5, toname);
    if (from_fid != NULL)
      {
      fclose(from_fid);
      from_fid = NULL;
      }
    return FALSE;
    }
  }

main_initialized = FALSE;    /* errors now are fatal */

/* Now initialize the first buffer */

main_bufferchain = store_Xget(sizeof(bufferstr));
init_buffer(main_bufferchain, 0, store_copystring(toname), store_copystring(toname),
  from_fid, to_fid, main_rmargin);
main_nextbufferno = 1;
init_selectbuffer(main_bufferchain, FALSE);
currentbuffer->filetype = filetype;
currentbuffer->streammax = stream;

main_filechanged = TRUE;
if ((fromname == NULL && toname == NULL) ||
    (fromname != NULL && toname != NULL && strcmp(fromname, toname) == 0 &&
      strcmp(fromname, "-") != 0))
        main_filechanged = FALSE;

cmd_stackptr = 0;
last_se = cmd_copyblock((cmdblock *)saved_se);  /* Currently, always NULL in fact */
last_gse = last_abese = NULL;
last_gnt = last_abent = NULL;
main_proclist = NULL;
cut_buffer = NULL;
cmd_cbufferline = NULL;
main_undelete = main_lastundelete = NULL;
main_undeletecount = 0;
par_begin = par_end = NULL;
files_written = NULL;

for (main_backptr = 0; main_backptr < back_size; main_backptr++)
  {
  (main_backlist[main_backptr]).line = NULL;
  }
main_backptr = 0;

/* In a non-windowing run, there may be additional file names,
to be loaded into other buffers. */

#ifdef Windowing
if (!main_windowing)
#endif

  {
  int i; 
  cmdstr cmd; 
  stringstr str; 
  bufferstr *firstbuffer = main_bufferchain;
  for (i = 0; i < 9; i++)
    {
    if (main_fromlist[i] == NULL) break;
    str.text = main_fromlist[i]; 
    cmd.arg1.string = &str;
    cmd.flags |= cmdf_arg1;
    if (e_newbuffer(&cmd) != done_continue) break;    
    }    
  init_selectbuffer(firstbuffer, TRUE);   
  } 

return TRUE;
}



/*************************************************
*          Given help on command syntax          *
*************************************************/

static void givehelp(void)
{
printf("NE version %s %s\n", version_string, version_date);
printf("-from       input file, default null, - means stdin, key can be omitted\n");
printf("-to         output file, default = from\n");
printf("-stream [n] run in stream mode, -to obligatory\n");
printf("-with       command file, default terminal\n");
printf("-ver        verification file, default screen\n");
printf("-id         show current version\n");
printf("-h[elp]     output this help\n");
printf("-line       run in line-by-line mode\n");
printf("-opt        initial line of commands\n");
printf("-noinit     don\'t obey %s line\n", sys_varname_NEINIT);
printf("-b[inary]   run in binary mode\n");
printf("-r[eadonly] start in readonly state\n");
printf("-tabs       expand input tabs; retab those lines on output\n");
printf("-tabin      expand input tabs; no tabs on output\n");
printf("-tabout     use tabs in all output lines\n");
printf("-notabs     no special tab treatment\n");

printf("          ENVIRONMENT VARIABLES\n");
printf("%s    contains initializing line of commands\n", sys_varname_NEINIT);
printf("%s    contains default tab option: tabs, tabin, or tabout\n", sys_varname_NETABS);

printf("          EXAMPLES\n");
printf("ne myfile -with commands -notabs\n");
printf("ne myfile -with commands -to outfile -stream\n");
}



/*************************************************
*             Decode command line                *
*************************************************/

/* The -W argument is permitted to keep rdargs happy, but
in fact it is dealt with elsewhere. */

static void decode_command(int argc, char **argv)
{
enum { arg_from,  arg_to=9,   arg_id,     arg_help,   arg_window,
       arg_line,  arg_with,   arg_ver,    arg_opt,    arg_noinit,
       arg_tabs,  arg_tabin,  arg_tabout, arg_notabs, arg_binary,
       arg_binw,  arg_stream, arg_readonly, arg_logging, arg_end };

int i, rc;
char argstring[256];
arg_result results[50];

strcpy(argstring,
  "from/9,to/k,id/s,help=h/s,W/s,line/s,with/k,ver/k,"
  "opt/k,noinit/s,tabs/s,tabin/s,tabout/s,notabs/s,binary=b/s,bw/s,stream/n=1000,"
  "readonly=r/s,errorlog/s");

rc = rdargs(argc, argv, sys_argstring(argstring), results);

results[arg_window].presence = results[arg_window].presence;  /* keep compiler happy */

if (rc != 0)
  {
  main_screenmode = main_screenOK = FALSE;
  error_moan(0, results[0].data.text, results[1].data.text);
  }

/* Deal with "id" and "help" */

if (results[arg_id].data.text != NULL)
  {
  printf("NE version %s %s\n", version_string, version_date);
  exit(EXIT_SUCCESS);
  }

if (results[arg_help].data.text != NULL)
  {
  givehelp();
  exit(EXIT_SUCCESS);
  }

/* Flag to skip obeying initialization string if requested */

if (results[arg_noinit].data.text != NULL) main_noinit = TRUE;

/* Handle tabbing options; the defaults are set from a system variable, but if
any options keywords are given, they define the entire state relative to
everything being FALSE. */

if (results[arg_tabs].data.text != NULL)
  { main_tabin = main_tabflag = TRUE; main_tabout = FALSE; }

if (results[arg_tabin].data.text != NULL)
  {
  main_tabin = TRUE;
  main_tabflag = FALSE;
  main_tabout = (results[arg_tabout].data.text != NULL);
  }

else if (results[arg_tabout].data.text != NULL)
  { main_tabin = FALSE; main_tabout = TRUE; }

if (results[arg_notabs].data.text != NULL)
  { main_tabin = main_tabout = FALSE; }

/* Force line-by-line mode if requested */

if (results[arg_line].data.text != NULL) 
  main_screenmode = main_screenOK = FALSE;

/* Binary options */

if (results[arg_binary].data.text != NULL) main_binary = main_overstrike = TRUE;
if (results[arg_binw].data.text != NULL)
  main_binary = main_binary_words = main_overstrike = TRUE;

/* Readonly option */

if (results[arg_readonly].data.text != NULL) main_readonly = TRUE;

/* Turn on logging option */

if (results[arg_logging].data.text != NULL) main_logging = TRUE;

/* Deal with an initial opt command line */

main_opt = results[arg_opt].data.text;

/* Set up a command file - this implies line-by-line mode */

if (results[arg_with].data.text != NULL)
  {
  main_screenmode = main_screenOK = main_interactive = FALSE;
  arg_with_name = store_copystring(results[arg_with].data.text);
  }

/* Set up a verification output file - this implies line-by-line mode */

if (results[arg_ver].data.text != NULL)
  {
  arg_ver_name = store_copystring(results[arg_ver].data.text);
  main_screenmode = main_screenOK = main_interactive = FALSE;
  if (strcmp(arg_ver_name, "-") != 0)
    {  
    msgs_fid = sys_fopen(arg_ver_name, "w", NULL);
    if (msgs_fid == NULL)
      {
      msgs_fid = stderr;
      error_moan(5, arg_ver_name);   /* Hard because not initialized */
      }
    } 
  }

/* Deal with any system-specific options */

sys_takeargs(results, arg_end);

/* Deal with "from" & "to" - the data must not be stored in E's dynamic store,
because that gets flushed when setting up for a new editing session in systems
that run under windows, e.g. RISC OS. We choose the simple way out, and use
some built-in buffers. */

if (results[arg_from].data.text != NULL)
  {
  arg_from_name = arg_from_buffer;
  strcpy(arg_from_name, results[arg_from].data.text);
  if (strcmp(arg_from_name, "-") == 0 &&  
    (arg_with_name == NULL || strcmp(arg_with_name, "-") == 0))
      {
      main_screenmode = FALSE;  
      error_moan(60, "-from or -with", "input");    /* Hard */
      } 
  }
  
/* Now deal with additional arguments to "from". There may be up to
9 in total (the maximum handled by rdargs). We have actually configured
main_fromlist for an additional 9, but mustn't read more than 8 extras
from the results list! (Otherwise we pick up other arguments...) */

for (i = 0; i < 9; i++) main_fromlist[i] = NULL;

for (i = 1; i < 9; i++)
  {
  if (results[arg_from + i].data.text == NULL) break;
  main_fromlist[i-1] = results[arg_from+i].data.text;
  }

/* Now "to" */

if (results[arg_to].data.text != NULL)
  {
  arg_to_name = arg_to_buffer;
  strcpy(arg_to_name, results[arg_to].data.text);
  if (strcmp(arg_to_name, "-") == 0)
    { 
    if (arg_ver_name == NULL) msgs_fid = stderr;
    else if (strcmp(arg_ver_name, "-") == 0)
      {
      main_screenmode = FALSE;  
      error_moan(60, "-to or -ver", "output");    /* Hard */
      } 
    } 
  }

/* Deal with the "stream" option: this requires that "to" also be given. */

if (results[arg_stream].data.number)
  {
  if (arg_to_name == NULL)
    {
    printf("** A -to argument must be given with -stream\n");
    exit(EXIT_FAILURE);
    }
  main_stream = results[arg_stream].data.number;
  }
}


/*************************************************
*             Initialize keystrings              *
*************************************************/

static void keystrings_init(void)
{
int i;
for (i = 0; i <= max_keystring; i++) main_keystrings[i] = NULL;
key_setfkey(1, "buffer");
/* key_setfkey(2, "window"); */
key_setfkey(3, "w");
key_setfkey(4, "undelete");
key_setfkey(6, "pll");
key_setfkey(7, "f");
key_setfkey(8, "m*");
key_setfkey(9, "show keys");
key_setfkey(10, "rmargin");
key_setfkey(11, "pbuffer");
/* key_setfkey(12, "closewindow"); */
key_setfkey(16, "plr");
key_setfkey(17, "bf");
key_setfkey(18, "m0");
key_setfkey(19, "show fkeys");
key_setfkey(20, "format");
key_setfkey(58, "topline");
key_setfkey(59, "back");
key_setfkey(60, "overstrike");
}


/*************************************************
*            Initialize tables                   *
*************************************************/

/* The character table is initialized dynamically, just in case we ever want
to port this to a non-ascii system. It has to be writable, because the user
can change the definition of what constitutes a "word". */

static void tables_init(void)
{
int i;
unsigned char *ucletters = (unsigned char *)"ABCDEFGHIJKLMNOPQRSTUVWXYZ";
unsigned char *lcletters = (unsigned char *)"abcdefghijklmnopqrstuvwxyz";
unsigned char *digits = (unsigned char *)"0123456789";
unsigned char *hexdigits = (unsigned char *)"0123456789ABCDEFabcdef";
unsigned char *delims = (unsigned char *)",.:\'\"!+-*/";
unsigned char *printing = (unsigned char *)" ABCDEFGHIJKLMNOPQRSTUVWXYZ"
  "abcdefghijklmnopqrstuvwxyz0123456789"
  ",.:\'\"!+-*/#$%&()=^~\\|@[{_`;]}<>?";

for (i = 0; i < 26; i++)
  {
  ch_tab[lcletters[i]] = ch_lcletter + ch_word;
  ch_tab[ucletters[i]] = ch_ucletter + ch_word;
  }
for (i = 0; i < 10; i++) ch_tab[digits[i]] = ch_digit + ch_word;
for (i = 0; i < 22; i++) ch_tab[hexdigits[i]] |= ch_hexch;
for (i = 0; i < (int)strlen((const char *)delims); i++)
  ch_tab[delims[i]] |= (ch_delim + ch_filedelim);

for (i = 0; i < (int)strlen(cmd_qualletters); i++)
  ch_tab[(unsigned char)(cmd_qualletters[i])] = 
  ch_tab[(unsigned char)(cmd_qualletters[i])] | ch_qualletter;

/* We have our own "is it printing?" table because various machines have
different ideas about high-numbered codes, and we want to stick to the
regular ASCII characters. */

for (i = 0; i < (int)strlen((const char *)printing); i++) 
  ch_tab2[printing[i]] = 1;

/* Table to translate a single letter key name into a "control
code". This table is used only in implementing the KEY command in
a way that is independent the machine's character set */

for (i = 0; i < 26; i++)
  {
  key_codes[ucletters[i]] = i+1;
  key_codes[lcletters[i]] = i+1;
  }
key_codes['\\']  = 28;
key_codes[']']   = 29;
key_codes['^']   = 30;
key_codes['_']   = 31;
}


/*************************************************
*            Run in line-by-line mode            *
*************************************************/

static void main_runlinebyline(void)
{
char *fromname = arg_from_name;
char *toname = (arg_to_name == NULL)? arg_from_name : arg_to_name;

if (main_interactive)
  {
  printf("NE version %s %s\n", version_string, version_date);
  main_verify = main_shownlogo = TRUE;
  }
else
  {
  main_verify = FALSE;
  if (arg_with_name != NULL && strcmp(arg_with_name, "-") != 0)
    {
    cmdin_fid = sys_fopen(arg_with_name, "r", NULL);
    if (cmdin_fid == NULL)
      error_moan(5, arg_with_name);  /* hard because not initialized */
    }
  }

arg_from_name = arg_to_name = NULL;
currentbuffer = main_bufferchain = NULL;

if (init_init(NULL, fromname, default_filetype, toname, main_stream))
  {
  static BOOL fgets_interrupted = FALSE;

  if (!main_noinit && main_einit != NULL) cmd_obey(main_einit);
  main_initialized = TRUE;
  if (main_opt != NULL) cmd_obey(main_opt);

  while (!main_done)
    {
    main_cicount = 0; 
    (void)main_interrupted(ci_read);
    if (main_verify) line_verify(main_current, TRUE, TRUE);
    signal(SIGINT, fgets_sigint_handler);
    if (main_interactive) main_rc = error_count = 0; 
    if (setjmp(rdline_env) == 0)
      {
      int n;
      if (cmdin_fid == NULL || fgets(cmd_buffer, cmd_buffer_size, cmdin_fid) == NULL)
        {
        strcpy(cmd_buffer, "w\n");
        }
      if (fgets_interrupted) sys_after_interrupted_fgets(TRUE, cmd_buffer);
      main_flush_interrupt();           /* resets handler */
      n = strlen(cmd_buffer);
      if (n > 0 && cmd_buffer[n-1] == '\n') cmd_buffer[n-1] = 0;
      (void)cmd_obey(cmd_buffer);
      fgets_interrupted = FALSE;
      }
    else
      {
      if (!main_interactive) break;
      clearerr(cmdin_fid);
      fgets_interrupted = TRUE;
      sys_after_interrupted_fgets(FALSE, NULL);
      }
    }

  if (cmdin_fid != NULL && cmdin_fid != stdin) fclose(cmdin_fid);
  }
}


/*************************************************
*          Set up default tab options            *
*************************************************/

/* All 3 flags are FALSE on entry */

static void tab_init(void)
{
if (main_tabs == NULL) return;
if (strcmp(main_tabs, "tabs") == 0) main_tabin = main_tabflag = TRUE;
else if (strcmp(main_tabs, "in") == 0) main_tabin = TRUE;
else if (strcmp(main_tabs, "out") == 0) main_tabout = TRUE;
else if (strcmp(main_tabs, "inout") == 0) main_tabin = main_tabout = TRUE;
}



/*************************************************
*                Entry Point                     *
*************************************************/

/* E can be run in an windowing environment, or as a command-line program.
The windowing environments are of course system-specific, but have the
characteristic that they are event-driven. Thus in such an environment
we hand over control to a system routine, and only get it back (if at all)
when the program is finishing. */

int main(int argc, char **argv)
{
int sigptr = 0;
kbd_fid = cmdin_fid = stdin;             /* defaults */
msgs_fid = stdout;

setvbuf(msgs_fid, NULL, _IONBF, 0);

tables_init();                 /* initialize tables */
keystrings_init();             /* set up default variable keystrings */

/* Need to detect whether windowing early on, as it may affect store
handling and initialization. Require the first argument to be -W for
a windowing run. */

#ifdef Windowing
main_windowing = argc > 1 && strcmp(argv[1], "-W") == 0;
#endif

sys_init1();                   /* Early local initialization */
version_init();                /* Set up for messages */
store_init();                  /* Initialize store handler */
tab_init();                    /* Set default tab options */
arg_zero = argv[0];            /* Some systems want this */
decode_command(argc, argv);    /* Decode command line */
sys_init2();                   /* Final local initialization */

cmd_buffer = malloc(cmd_buffer_size);

if ((arg_from_name != NULL && strcmp(arg_from_name, "-") == 0) ||
    (arg_to_name != NULL && strcmp(arg_to_name, "-") == 0))
      main_interactive = main_screenmode = main_screenOK = FALSE;

/* Set handler to trap keyboard interrupts */

signal(SIGINT, sigint_handler);

/* Set handler for all other relevant signals to dump buffers and exit
from E. The list of signals is system-dependent. */

while (signal_list[sigptr] > 0) signal(signal_list[sigptr++], crash_handler);


/* If windowing, give control to the wimp system. If not windowing, the
variables main_screenmode and main_interactive will have been set to
indicate the style of editing run. */

#ifdef Windowing
if (main_windowing) { cmdin_fid = NULL; sys_runwindow(); } else
#endif
  if (main_screenmode) { cmdin_fid = NULL; sys_runscreen(); }
    else main_runlinebyline();


if (debug_file != NULL) fclose(debug_file);
if (crash_logfile != NULL) fclose(crash_logfile);


#ifdef Windowing
if (main_screenOK && !main_windowing && main_nlexit && main_pendnl)
#else
if (main_screenOK && main_nlexit && main_pendnl)
#endif
  sys_mprintf(msgs_fid, "\r\n");

return sys_rc(main_rc);
}

/* End of einit.c */
