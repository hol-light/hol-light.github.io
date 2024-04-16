/***********************************************************
*             The E text editor - 2nd incarnation          *
***********************************************************/

/* Copyright (c) University of Cambridge, 1991 - 2003 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: March 2003 */

/************************************************************
* E is a text editor that is designed to run on a variety   *
* of hardware and under a variety of operating systems. The *
* first implementation was written BCPL to run under        *
* Phoenix/MVS. This is a complete re-write in C, undertaken *
* because BCPL is now dying. It no longer runs under MVS,   *
* and it is now called NE.                                  *
************************************************************/


/* This is the main header file, imported by all other sources. */


/***********************************************************
*                    Includes of other headers             *
***********************************************************/

#include <ctype.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>     /* last for SunOS' benefit (saves NULL redefined warnings) */

/* Include anything OS-specific */

#include "elocal.h"


/***********************************************************
*         Macros for differing C systems & word sizes      *
***********************************************************/

/* Note the existence of elocal.h for OS-specific things */


/******** Borland C++ 3.1 ********/

#if defined(__BORLANDC__) && !defined(__WIN32CON__)
#define BIGNUMBER 0x7fff
#define MAX_RMARGIN 16383
#define LONGINT long
#define HUGE huge

/******** The default is for a 32-bit system ********/

#else
#if !defined(__WIN32CON__)
  #define min(a,b) (((a) < (b))? (a) : (b))
#endif
#define BIGNUMBER 0x7fffffff
#define MAX_RMARGIN 1000000
#define LONGINT int
#define HUGE
#endif





/***********************************************************
*                  Global Macros                           *
***********************************************************/


#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE  1
#endif

#define BOOL  int
#define CBOOL char               /* used to save space in structures */

#define intbits      (sizeof(int)*8)     /* number of bits in an int */
#define qsmapsize   (32/sizeof(int))     /* number of bytes in a 256-bit map */

#define message_window      1
#define first_window        2

#define max_fkey           30    /* max function key */
#define max_keystring      60    /* max function keystring */
#define max_fixedKstring   10    /* unchangeable ones */
#define max_undelete      100    /* maximum undelete lines */
#define max_wordlen        19

#define stream_default   1000    /* default line limit for streams */

#define back_size          20    /* number of "back" references */
#define JournalSize      4096

#define cmd_buffer_size   512
#define cmd_stacktop       20


#define mac_nextline(line) (((line)->next == NULL)? file_extend() : (line)->next)

#define mac_skipspaces(a)  while (*a == ' ') a++

/* Graticules flags */

#define dg_none        0  /* nothing to be drawn */
#define dg_both        1  /* re-draw both */
#define dg_bottom      2  /* re-draw bottom one only */
#define dg_flags       4  /* re-draw flag info only */
#define dg_margin      8  /* re-draw margin info */
#define dg_top        16  /* re-draw top only */


/* Character types */

#define ch_ucletter     0x01
#define ch_lcletter     0x02
#define ch_letter       0x03
#define ch_digit        0x04
#define ch_qualletter   0x08
#define ch_delim        0x10
#define ch_word         0x20
#define ch_hexch        0x40
#define ch_filedelim    0x80

/* Flags for certain qualifier strings on replacements */

#define rqs_XRonly       1
#define rqs_Xonly        2

/* Scroll flags for windowing scrolls */

#define wscroll_up       0
#define wscroll_down     1
#define wscroll_left     2
#define wscroll_right    3

/* Flags for the if command */

#define if_prompt        1
#define if_mark          2
#define if_eol           4
#define if_sol           8
#define if_sof          16
#define if_if           32
#define if_unless       64   /* Unless must be last! */



/***********************************************************
*                     Structures                           *
***********************************************************/

/* The structures are held in a separate file for convenience */

#include "structs.h"



/***********************************************************
*                    Typedefs                              *
***********************************************************/

typedef void (HUGE *cmd_Cproc)(cmdstr *);      /* The type of compile procedures */
typedef int  (HUGE *cmd_Eproc)(cmdstr *);      /* The type of execute procedures */



/***********************************************************
*                    Enumerations                          *
***********************************************************/

enum { lb_align, lb_delete, lb_eraseright, lb_eraseleft, lb_closeup,
       lb_closeback, lb_rectsp, lb_alignp };

enum { mark_unset, mark_lines, mark_text, mark_rect, mark_global };

enum { amark_line, amark_limit, amark_text, amark_rectangle, amark_unset, amark_hold };

enum { ktype_data, ktype_ctrl, ktype_function };

enum { done_continue, done_error, done_finish, done_wait, done_loop,
       done_break, done_eof };

enum { sh_insert, sh_topline, sh_above };

enum { cuttype_text, cuttype_rect };

enum { show_ckeys = 1, show_fkeys, show_xkeys, show_allkeys, 
  show_keystrings, show_buffers, show_wordcount, show_version, 
  show_actions, show_commands, show_wordchars, show_settings };

enum { abe_a, abe_b, abe_e };

enum { cbuffer_c, cbuffer_cd };

enum { set_autovscroll = 1, set_splitscrollrow, set_oldcommentstyle,
  set_newcommentstyle };

enum { debug_crash = 1, debug_exceedstore, debug_nullline, debug_baderror };

enum { save_keepname = 1 };

enum { detrail_buffer, detrail_output };

enum { backup_files };

enum { ci_move, ci_type, ci_read, ci_cmd, ci_delete, ci_scan, ci_loop };

enum { of_other, of_existence };




/***********************************************************
*                  Global variables                        *
***********************************************************/

extern FILE *crash_logfile;            /* For logging info on crashes */
extern FILE *debug_file;               /* For general debugging */

extern FILE *cmdin_fid;
extern FILE *from_fid;
extern FILE *msgs_fid;
extern FILE *kbd_fid;

extern char *arg_from_name;            /* Names on command line */
extern char *arg_to_name;
extern char *arg_ver_name;
extern char *arg_with_name;
extern char *arg_zero;                 /* ARGV[0] */

extern bufferstr *currentbuffer;

extern char *ch_tab;                   /* table for identifying char types */
extern char *ch_tab2;                  /* and regular printing characters */

extern cmd_Cproc cmd_Cproclist[];
extern cmd_Eproc cmd_Eproclist[];

extern int   cmd_bracount;             /* Bracket level */
extern int   cmd_breakloopcount;       /* Break count */
extern char *cmd_buffer;               /* Buffer for reading cmd lines */
extern BOOL  cmd_casematch;            /* Case match switch */
extern linestr *cmd_cbufferline;       /* Next line in cbuffer */
extern int   cmd_clineno;              /* Line number in C or CBUFFER */
extern char *cmd_cmdline;              /* Current command line being compiled */
extern char *cmd_list[];               /* List of command names */
extern int   cmd_listsize;             /* Size of command list */
extern BOOL  cmd_faildecode;           /* Failure flag */
extern int   cmd_ist;                  /* Delimiter for inserting in cmd concats */
extern BOOL  cmd_eoftrap;              /* Trapping eof */
extern BOOL  cmd_onecommand;           /* Command line is a single command */
extern char *cmd_ptr;                  /* Current pointer */
extern BOOL  cmd_refresh;              /* Refresh needed */
extern char *cmd_qualletters;          /* Qualifier letters */
extern char *cmd_stack[];              /* Stack of old command lines */
extern int   cmd_stackptr;             /* Pointer in command stack */
extern char  cmd_word[];               /* Word buffer */

extern BOOL  crash_handler_chatty;     /* To supress for SIGHUP */

extern int   cursor_row;               /* Relative screen row */
extern int   cursor_col;               /* Absolute column within line */
extern int   cursor_max;               /* Max abs value on current screen */
extern int   cursor_rh_adjust;         /* Adjust for auto scrolling */
extern int   cursor_offset;            /* Left most screen column */

extern linestr *cut_buffer;            /* start of cut buffer */
extern linestr *cut_last;              /* last line in same */
extern BOOL     cut_pasted;            /* set when buffer is pasted */
extern int      cut_type;              /* type of last cut */

extern int   default_filetype;         /* For systems that use types */
extern int   default_rmargin;          /* For making/reinitializing windows */

extern int     error_count;
extern jmp_buf error_jmpbuf;           /* For disastrous errors */
extern BOOL    error_longjmpOK;        /* Set when longjmp can be taken */
extern BOOL    error_werr;             /* Force window-type error */

extern filewritstr *files_written;     /* Chain of written file names */

extern char *key_actionnames[];
extern int   key_actnamecount;         /* size of following table */
extern keynamestr key_actnames[];      /* names for key actions */
extern char  key_codes[256];
extern LONGINT  key_controlmap;        /* map for existing keys */
extern short int key_fixedtable[];     /* tables for interpreting keys */
extern LONGINT  key_functionmap;
extern keynamestr key_names[];         /* names for special keys */
extern LONGINT  key_specialmap[4];
extern char *key_specialnames[];
extern short int key_table[];

extern sestr *last_se;                 /* last search expression */
extern sestr *last_abese;              /* last se for abe */
extern qsstr *last_abent;              /* last replacement for abe */
extern sestr *last_gse;                /* last se for global */
extern qsstr *last_gnt;                /* last replacement qs for global */

extern linestr *main_bottom;           /* last line */
extern linestr *main_current;          /* current line */
extern linestr *main_top;              /* first line */

extern bufferstr *main_bufferchain;

extern BOOL  main_appendswitch;        /* cut append option */
extern BOOL  main_attn;                /* attention on/off switch */
extern BOOL  main_AutoAlign;
extern backstr *main_backlist;         /* list of "back" positions */
extern int   main_backprevptr;         /* previous "back" move */
extern int   main_backptr;             /* position in list */
extern BOOL  main_backupfiles;         /* flag for auto-backing up */
extern BOOL  main_binary;              /* handle lines as binary */
extern BOOL  main_binary_words;        /* ... and as words */
extern int   main_cicount;             /* check interrupt count */
extern int   main_currentlinenumber;   /* relative number of current line */
extern BOOL  main_detrail_output;      /* detrail all output lines */
extern BOOL  main_done;                /* job is finished */
extern int   main_drawgraticules;      /* an option value */
extern BOOL  main_eightbit;            /* display option */
extern char *main_einit;               /* initializing options from environment */
extern BOOL  main_eoftrap;             /* until eof flag */
extern BOOL  main_escape_pressed;      /* a human interrupt */
extern char *main_fromlist[];          /* additional input files */
extern BOOL  main_initialized;         /* initial cmds obeyed */
extern BOOL  main_filechanged;         /* file has been edited */
extern char *main_filealias;
extern char *main_filename;
extern char *main_fixedKstrings[];     /* fixed key strings */
extern int   main_hscrollamount;       /* left/right scroll value */
extern int   main_ilinevalue;          /* boundary between up/down scroll */
extern BOOL  main_interactive;         /* set if interactive */
extern int   main_imax;                /* number of last line read */
extern int   main_imin;                /* number of last insert */
extern int   main_linecount;           /* number of lines in current buffer */
extern BOOL  main_logging;             /* turns on debugging logging */
extern BOOL  main_nlexit;              /* needs NL on exit */
extern BOOL  main_noinit;              /* don't obey init string */
extern char *main_keystrings[];        /* variable keystrings */
extern linestr *main_lastundelete;     /* last undelete structure */
extern BOOL  main_leave_message;       /* leave msg in bottom window after cmds */
extern int   main_nextbufferno;        /* next number to use */
extern int   main_oldcomment;          /* old-style comments flag */
extern BOOL  main_nowait;              /* no wait before screen refresh */
extern BOOL  main_oneattn;             /* had one suppressed attention */
extern char *main_opt;                 /* the "opt" argument */
extern int   main_optscroll;           /* scroll count */
extern BOOL  main_overstrike;
extern BOOL  main_pendnl;              /* pending nl if more line-by-line output */
extern procstr *main_proclist;         /* procedure chain */
extern int   main_rc;                  /* The final return code */
extern BOOL  main_readonly;            /* Buffer is read only */
extern BOOL  main_repaint;             /* Force screen repaint after command */
extern int   main_rmargin;             /* current margin */
extern BOOL  main_screenmode;          /* true if full-screen operation */
extern BOOL  main_screenOK;            /* screen mode and not suspended */
extern BOOL  main_screensuspended;     /* screen temporarily suspended */
extern int  *main_screentabs;          /* list of tab stops */
extern BOOL  main_selectedbuffer;      /* true if buffer has changed */
extern BOOL  main_shownlogo;           /* FALSE if need to show logo on error */
extern LONGINT  main_storetotal;       /* Total store used */
extern int   main_stream;              /* -stream option value */
extern BOOL  main_tabflag;             /* Flag tabbed input lines */
extern BOOL  main_tabin;               /* the tabin option */
extern BOOL  main_tabout;              /* the tabout option */
extern char *main_tabs;                /* the default tabs text option */
extern linestr *main_undelete;         /* first undelete structure */
extern int   main_undeletecount;       /* count of lines */
extern BOOL  main_unixregexp;          /* Unix regular expressions */
extern int   main_vcursorscroll;       /* Vertical scroll amount */
extern BOOL  main_verified_ptr;        /* Non-null > has been output */
extern BOOL  main_verify;              /* Auto verification */
extern BOOL  main_warnings;            /* warnings option */

#ifdef Windowing
extern BOOL  main_windowing;           /* TRUE if running in a window system */
#endif


extern int   mark_col;
extern BOOL  mark_hold;
extern int   mark_type;
extern linestr *mark_line;

extern int   match_end;
extern BOOL  match_L;                  /* Leftwards match */
extern int   match_leftpos;
extern int   match_start;
extern int   match_rightpos;

extern sestr *par_begin;
extern sestr *par_end;

extern unsigned char *R_Journal;       /* Set up by RE callers */

extern sestr *saved_se;                /* Saved over windowing quiesce */

extern BOOL  screen_autoabove;
extern BOOL  screen_forcecls;          /* Force a complete refresh */
extern BOOL  screen_suspend;           /* Set to cause suspension over * commands */
extern BOOL  screen_use_scroll;        /* Use scroll to speed up displaying */

extern int   screen_max_col;           /* Applies to full screen */
extern int   screen_max_row;

extern int   signal_list[];            /* Crash signals */
extern char *signal_names[];           /* and their names */

extern int   sys_openfail_reason;      /* Reason for open failure */
extern char *sys_varname_NEINIT;       /* For -help output */
extern char *sys_varname_NETABS;

extern int   topbit_minimum;           /* lowest top-bit char */

extern char *version_date;             /* Identity date */
extern char *version_string;           /* Identity of program version */

extern linestr **window_vector;        /* points to vector of lines being displayed */

extern int  window_bottom;
extern int  window_depth;
extern int  window_top;
extern int  window_width;


/***********************************************************
*                    Global procedures                     *
***********************************************************/

extern void  debug_printf(char *, ...);
extern void  debug_screen(void);
extern void  debug_writelog(char *, ...);

extern BOOL  cmd_atend(void);
extern cmdstr *cmd_compile(void);
extern int   cmd_confirmoutput(char *, BOOL, BOOL, BOOL, int, char **);
extern void *cmd_copyblock(cmdblock *);
extern BOOL  cmd_emptybuffer(bufferstr *, char *);
extern bufferstr *cmd_findbuffer(int);
extern BOOL  cmd_findproc(char *, procstr **);
extern void  cmd_freeblock(cmdblock *);
extern cmdstr *cmd_getcmdstr(int);
extern BOOL  cmd_joinline(BOOL);
extern BOOL  cmd_makeFSM(qsstr *);
extern BOOL  cmd_matchqsR(qsstr *, linestr *, int);
extern BOOL  cmd_matchse(sestr *, linestr *, int);
extern int   cmd_obey(char *);
extern int   cmd_obeyline(cmdstr *);
extern int   cmd_readnumber(void);
extern BOOL  cmd_readprocname(stringstr **name);
extern BOOL  cmd_readqualstr(qsstr **, int);
extern int   cmd_readrestofline(stringstr **);
extern BOOL  cmd_readse(sestr **);
extern int   cmd_readstring(stringstr **);
extern int   cmd_readUstring(stringstr **);
extern void  cmd_readword(void);
extern linestr *cmd_ReChange(linestr *, char *, int, BOOL, BOOL, BOOL, BOOL *);
extern void  cmd_recordchanged(linestr *, int);
extern void  cmd_saveback(void);
extern BOOL  cmd_yesno(char *, ...);

extern void  crash_handler(int);

extern BOOL  cut_cut(linestr *, int, int, BOOL, BOOL);
extern void  cut_pasterect(void);
extern BOOL  cut_overwrite(char *);
extern int   cut_pastetext(void);

extern void  error_moan(int, ...);
extern void  error_moanqse(int, sestr *);
extern void  error_printf(char *, ...);
extern void  error_printflush(void);

extern linestr *file_extend(void);
extern linestr *file_nextline(FILE **, int *, bufferstr *);
extern BOOL     file_save(char *);
extern void     file_setwritten(char *);
extern BOOL     file_streamout(char *);
extern BOOL     file_written(char *);
extern int      file_writeline(linestr *, FILE *);

extern void     init_buffer(bufferstr *, int, char *, char *, FILE *, FILE *, int);
extern BOOL     init_init(FILE *, char *, int, char *, BOOL);
extern void     init_selectbuffer(bufferstr *, BOOL);

extern int      line_checkabove(linestr *);
extern linestr *line_concat(linestr *, int);
extern linestr *line_copy(linestr *);
extern linestr *line_cutpart(linestr *, int, int, BOOL);
extern linestr *line_delete(linestr *, BOOL);
extern void     line_deletech(linestr *, int, int, BOOL);
extern void     line_formatpara(void);
extern void     line_insertch(linestr *, int, char *, int, int);
extern void     line_leftalign(linestr *, int, int *);
extern linestr *line_split(linestr *, int);
extern void     line_verify(linestr *, BOOL, BOOL);

extern void  key_handle_data(int);
extern void  key_handle_function(int);
extern void  key_makespecialname(int, char *);
extern int   key_set(char *, BOOL);
extern void  key_setfkey(int, char *);

extern void  main_flush_interrupt(void);
extern BOOL  main_interrupted(int);
extern void  main_checkstream(void);

extern int rdargs(int, char **, char *, arg_result *);

extern void  scrn_afterhscroll(void);
extern void  scrn_display(BOOL);
extern void  scrn_displayline(linestr *, int, int);
extern void  scrn_display_linenumber(void);
extern void  scrn_hint(int, int, linestr *);
extern void  scrn_init(BOOL);
extern void  scrn_invertchars(linestr *, int, int, int, int);
extern void  scrn_rdline(BOOL, BOOL, char *);
extern void  scrn_restore(void);
extern void  scrn_scrollby(int);
extern void  scrn_setsize(void);
extern void  scrn_suspend(void);
extern void  scrn_terminate(void);
extern void  scrn_windows(void);

extern int   setup_dbuffer(bufferstr *);
extern int   setup_newbuffer(char *);

extern void  store_chop(void *, LONGINT);
extern void *store_copy(void *);
extern linestr *store_copyline(linestr *);
extern char *store_copystring(char *);
extern char *store_copystring2(char *, char *);
extern void  store_free(void *);
extern void  store_freequeuecheck(void);
extern void  store_free_all(void);
extern void *store_get(LONGINT);
extern void *store_getlbuff(LONGINT);
extern void  store_init(void);
extern void *store_Xget(LONGINT);

extern void  sys_after_interrupted_fgets(BOOL, char *);
extern char *sys_argstring(char *);
extern void  sys_beep(void);
extern char *sys_checkfilename(char *, BOOL);
extern void  sys_checkinterrupt(int);
extern int   sys_cmdkeystroke(int *);
extern char *sys_crashfilename(int);
extern void  sys_crashposition(void);
extern int   sys_crlfcount(void);
extern void  sys_display_cursor(int);
extern int   sys_fcomplete(char *, int, int *);
extern FILE *sys_fopen(char *, char *, int *);
extern BOOL  sys_help(char *);
extern void  sys_hourglass(BOOL);
extern void  sys_init1(void);
extern void  sys_init2(void);
extern char *sys_keyreason(int);
extern void  sys_keystroke(int);
extern BOOL  sys_makespecialname(int, char *);
extern void  sys_mprintf(FILE *, char *, ...);
extern int   sys_rc(int);
extern void  sys_runscreen(void);
extern void  sys_runwindow(void);
extern void  sys_setfiletype(char *, int);
extern void  sys_setversion(char *);
extern void  sys_specialnotes(void);
extern void  sys_takeargs(arg_result*, int);
extern void  version_init(void);

/* Function variables */

extern void  (HUGE *sys_resetterminal)(void);
extern void  (HUGE *sys_setupterminal)(void);

/* Interface to windowing systems */

#ifdef Windowing
extern void  sys_werr(char *);
extern int   sys_window_close(bufferstr *);
extern void  sys_window_close_all(void);
extern void  sys_window_cmdline(char *);
extern void  sys_window_deletelines(int, int);
extern void  sys_window_diag(BOOL);
extern void  sys_window_display(BOOL, BOOL);
extern void  sys_window_display_line(int, int);
extern void  sys_window_display_linenumber(char *);
extern void  sys_window_display_text(char *);
extern void  sys_window_info(int *);
extern void  sys_window_insertlines(int, int);
extern void  sys_window_load(void);
extern void  sys_window_refresh(void);
extern void  sys_window_reshow(void);
extern void  sys_window_scroll(int);
extern void  sys_window_selectbuffer(bufferstr *);
extern void  sys_window_showmark(int, int);
extern int   sys_window_start_off(char *);
extern void  sys_window_topline(int);
extern BOOL  sys_window_yesno(BOOL, char *, char *, ...);
#endif

/* End of ehdr.h */
