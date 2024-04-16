/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1999 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: December 1999 */

/* This file contains all the structure definitions, together with parameters
that control the size of some of them. */


#define backstacksize 20


/* Result from rdargs routine */

typedef struct {
  LONGINT presence;
  union {
    LONGINT number;
    char *text;
  } data;
} arg_result;

enum { arg_present_not, arg_present_unkeyed, arg_present_keyed };


/* Line */

typedef struct line {
  struct line *next;         /* chain to next */
  struct line *prev;         /* chain to previous */
  char         *text;        /* the characters themselves */
  int          key;          /* line number */
  short int    len;          /* number of chars */
  char         flags;        /* various flag bits */
} linestr;

/* Bits in line flags byte */

#define lf_eof     1         /* eof line */
#define lf_shn     2         /* show needed */
#define lf_clend   4         /* clear out end of line */
#define lf_shm     8         /* show margin char only */
#define lf_shbits 14         /* show request bits */
#define lf_tabs   16         /* expanded tabs in this line */
#define lf_udch   32         /* chars for undelete */
#define lf_cmd    64         /* was a taskwindow cmd line (RISC OS) */


/* Entry in "back" vector */

typedef struct {
  linestr *line;
  int col;
} backstr;


/* Buffer */

typedef struct buffer {
  struct buffer *next;       /* next buffer block */

  linestr *bottom;           /* last line in buffer */
  linestr *current;          /* current line in buffer */
  linestr *markline;         /* the line with the mark */
  linestr *scrntop;          /* saving top line on screen */
  linestr *top;              /* first line in buffer */

  backstr *backlist;         /* vector of saved positions */

  int backprevptr;
  int backptr;               /* position in list */
  int binoffset;             /* offset for reading file in binary */
  int bufferno;              /* buffer number */
  int col;                   /* cursor column */
  int commanding;            /* count of active cbuffer commands */ 
  int currentlinenumber;     /* number of current line in buffer */
  int filetype;              /* for systems needing it */ 
  int imax;                  /* max input line number */
  int imin;                  /* max inserted line (-ve) number */
  int linecount;             /* number of lines in the buffer */
  int markcol;               /* the mark colume */
  int marktype;              /* the mark's type */
  int offset;                /* the cursor offset */
  int row;                   /* cursor row */
  int rmargin;               /* right margin */
  int streammax;             /* max lines for stream buffers */ 
  
  char *filealias;           /* name to display */
  char *filename;            /* real name */

  /* The only windowing version currently supported is for the Acorn 
  RiscOS operating system. If any others ever get supported (not very 
  likely) other fields might be needed. */
   
  #ifdef Windowing 
  int windowhandle;          /* window handle */
  int windowhandle2;         /* further data if needed */ 
  int skipcount;             /* skip output in taskwindow */
  int outcursor;             /* output cursor got to here */
  linestr *lastarrow;        /* last up/down arrow in taskwindow */    
  char *windowtitle;         /* window title bar text */ 
  CBOOL cursor_shown;        /* cursor is currently displayed */ 
  CBOOL lastwascr;           /* for taskwindows */ 
  CBOOL killed;              /* for taskwindows */
  #endif 

  CBOOL changed;             /* buffer edited */
  CBOOL cleof;               /* close file at eof */
  CBOOL noprompt;            /* no prompting wanted */
  CBOOL readonly;            /* readonly flag */ 
  CBOOL saved;               /* saved to "own" file */

  FILE *from_fid;            /* input to this buffer */
  FILE *to_fid;              /* last output from this buffer */
} bufferstr;



/* === Control blocks for command processing === */

/* The first byte of all these blocks is called "type" and identifies the kind
of block. The various types are: */

#define  cb_setype  1            /* search expression */
#define  cb_qstype  2            /* qualified string */
#define  cb_sttype  3            /* plain string */
#define  cb_cmtype  4            /* command block */
#define  cb_iftype  5            /* 'if' 2nd arg block */
#define  cb_prtype  6            /* procedure */
#define  cb_tbtype  7            /* tab list */

/* Here is a generic structure for addressing the type field */

typedef struct {
  char type;
} cmdblock;


/* Qualified string */

typedef struct qsstr {
  char type;                   /* cb_qstype */
  char count;                  /* count qualifier */
  short int flags;             /* see below */
  short int windowleft;        /* window values */
  short int windowright;       
  short int length;            /* length of original string */
  short int rlength;           /* length of (possibly converted) RE string */
                               /* unused for PCRE regexs */ 
  unsigned char *fsm;          /* finite state machine for R */
                               /* or pointer to pcre block for PCRE use */ 
  char *hexed;                 /* hexed chars for non-R */
  char *rtext;                 /* converted RE string */ 
                               /* unused for PCRE regexs */ 
  char *text;                  /* data chars */
  unsigned int map[qsmapsize]; /* bit map for contained chars */
} qsstr;

/*  Search Expression; its left/right pointers can point to either
another search expression or a qualified string. Hence the messing
about with the union to achieve this. */

struct sestr;

typedef union {
  struct qsstr *qs;
  struct sestr *se;
} qseptr;

typedef struct sestr {
  char type;                 /* cb_setype */
  char count;                /* count qualifier */
  short int flags;           /* see below */
  short int windowleft;      /* window values */
  short int windowright;
  qseptr left;               /* left subtree */
  qseptr right;              /* right subtree */
} sestr;


/* Default window values */

#define qse_defaultwindowleft   0
#define qse_defaultwindowright  0x7fff

/* Qualifier flags; 16-bit field */

#define qsef_B      0x0001
#define qsef_E      0x0002
#define qsef_H      0x0004
#define qsef_L      0x0008
#define qsef_N      0x0010
#define qsef_R      0x0020
#define qsef_S      0x0040
#define qsef_U      0x0080
#define qsef_V      0x0100
#define qsef_W      0x0200
#define qsef_X      0x0400
#define qsef_AND    0x0800  /* AND flag for se nodes */
#define qsef_REV    0x1000  /* Reversed flag for regular expressions */
#define qsef_RP     0x2000  /* PCRE regular expression */
#define qsef_FV     0x4000  /* PCRE regex, compiled verbatim by USW */

/* Both of E and B */
#define qsef_EB     (qsef_E + qsef_B)

/* Not allowed in search expressions */
#define qsef_NotSe  (qsef_REV+qsef_AND+qsef_X+qsef_R+qsef_L+qsef_H+qsef_E+qsef_B)

/* Plain String */

typedef struct {
  char type;
  char delim;
  char hexed;
  char *text;
} stringstr;

/* Command structures and arguments are mutually recursive */

struct cmd;

/* Structure for if and unless */

typedef struct {
  char type; /* cb_iftype */
  struct cmd *if_then;
  struct cmd *if_else;
} ifstr;

/* Union type for command arguments */

typedef union {
  void *block;
  stringstr *string;
  qsstr *qs;
  sestr *se;
  int value;
  struct cmd *cmds;
  ifstr *ifelse;
} cmdarg;

/* Command block */

typedef struct cmd {
  char type;
  char id;                  /* identity of command */
  char flags;               /* argument flags */
  char misc;                /* switch for miscellaneous cmd options */
  char ptype1;              /* prompt type for arg1 */
  char ptype2;              /* prompt type for arg2 */
  char arg1type;            /* type of arg1 */
  char arg2type;            /* type of arg2 */

  struct cmd *next;         /* next cmd on chain */
  int  count;               /* repeat count */

  cmdarg arg1;              /* 1st argument */
  cmdarg arg2;              /* 2nd argument */
} cmdstr;


/* Flag values */

#define  cmdf_arg1   1  /* arg1 present */
#define  cmdf_arg2   2  /* arg2 present */
#define  cmdf_arg1F  4  /* arg1 is ptr to control block */
#define  cmdf_arg2F  8  /* arg2 is ptr to control block */
#define  cmdf_group 16  /* this is cmd group */


/* Procedure structure */

typedef struct procstr {
  char   type;
  char   flags;
  char   *name;
  cmdstr *body;
  struct procstr *next;
} procstr;

#define pr_active        1



/* Layout of key names table entries */

typedef struct {
  char *name;
  int   code;
} keynamestr;

/* Structure for remembering the names of written files */

typedef struct filewritten {
  struct filewritten *next;
  char *name;
} filewritstr;    


/* End of structs.h */
