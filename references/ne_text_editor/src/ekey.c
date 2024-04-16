/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1999 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: July 1999 */


/* This file contains code for handling individual function keystrokes. */


#include "ehdr.h"
#include "cmdhdr.h"
#include "shdr.h"
#include "scomhdr.h"
#include "keyhdr.h"

static char key_readonly[] = {
  0, /* ka_al */
  0, /* ka_alp */
  0, /* ka_cl */
  0, /* ka_clb */
  1, /* ka_co */
  1, /* ka_csd */
  1, /* ka_csl */
  1, /* ka_csls */
  1, /* ka_csle */
  1, /* ka_csnl */
  1, /* ka_cstl */
  1, /* ka_cstr */
  1, /* ka_csr */
  1, /* ka_cssbr */
  1, /* ka_cssl */
  1, /* ka_csstl */
  1, /* ka_cstab */
  1, /* ka_csptab */
  1, /* ka_csu */
  1, /* ka_cswl */
  1, /* ka_cswr */
  0, /* ka_cu */
  0, /* ka_dal */
  0, /* ka_dar */
  0, /* ka_dc */
  0, /* ka_de */
  0, /* ka_dl */
  0, /* ka_dp */
  0, /* ka_dtwl */
  0, /* ka_dtwr */
  1, /* ka_gm */
  0, /* ka_join */
  1, /* ka_lb */
  0, /* ka_pa */
  1, /* ka_rb */
  1, /* ka_reshow */
  1, /* ka_rc */
  0, /* ka_rs */
  1, /* ka_scbot */
  1, /* ka_scdown */
  1, /* ka_scleft */
  1, /* ka_scright */
  1, /* ka_sctop */
  1, /* ka_scup */
  0, /* ka_split */
  1, /* ka_tb */
  0, /* ka_dpleft */
  1, /* ka_forced */
  0, /* ka_last */
  0, /* ka_ret */
  1, /* ka_wbot */
  1, /* ka_wleft */
  1, /* ka_wright */
  1  /* ka_wtop */
};



/*************************************************
*            Display state of the mark           *
*************************************************/

static void ShowMark(void)
{
#ifdef Windowing
if (main_windowing)
  {
  sys_window_showmark(mark_type, mark_hold);
  return;
  }
#endif

if (mark_type != mark_unset)
  {
  s_selwindow(message_window, -1, -1);
  s_cls();
  switch (mark_type)
    {
    case mark_lines: s_printf("Bulk line operation%s started", mark_hold? "s":""); break;
    case mark_text: s_printf("Text block started"); break;
    case mark_rect: s_printf("Rectangular block started"); break;
    case mark_global: s_printf("Global limit set"); break;
    }
  }
}


/*************************************************
*               Cancel Mark                      *
*************************************************/

static void CancelMark(void)
{
int old_window = s_window();

#ifdef Windowing
if (!main_windowing)
#endif

if (mark_line != NULL)
  {
  int i;
  int row = -1;
  for (i = 0; i <= window_depth; i++) if (mark_line == window_vector[i])
    {
    row = i;
    break;
    }
  if (row >= 0 && mark_col >= cursor_offset && mark_col < cursor_max)
    {
    if ((mark_line->flags & lf_eof) != 0) mark_line->flags |= lf_shn;
      else scrn_invertchars(mark_line, row, mark_col, 1, FALSE);
    }
  }

mark_type = mark_unset;
mark_line = NULL;

/* Remove comment from message window */

s_selwindow(message_window, -1, -1);
s_cls();
s_selwindow(old_window, -1, -1);
}



/*************************************************
*               Set Mark                         *
*************************************************/

static int SetMark(int marktype)
{
if (mark_type == mark_unset)
  {
  mark_line = main_current;
  mark_col = cursor_col;
  mark_type = marktype;
  mark_hold = FALSE;
  scrn_invertchars(main_current,cursor_row,cursor_col,1,TRUE);
  return TRUE;
  }
if (mark_type == marktype) CancelMark();
return FALSE;
}



/*************************************************
*              Change keystring value            *
*************************************************/

/* Keystring values must be held in non-volatile store */

void key_setfkey(int n, char *s)
{
char *news;
if (s == NULL) news = NULL; else
  {
  news = malloc(strlen(s) + 1);
  strcpy(news, s);
  }
if (main_keystrings[n] != NULL) free(main_keystrings[n]);
main_keystrings[n] = news;
}




/*************************************************
*             Obey a command line                *
*************************************************/

static void key_obey_commands(char *cmdline)
{
#ifdef Windowing
if (main_windowing) { (void)cmd_obey(cmdline); return; }
#endif

for (;;)
  {
  main_pendnl = TRUE;
  main_nowait = main_repaint = FALSE;
  if ((cmd_obey(cmdline) == done_wait || !main_pendnl || main_repaint) && !main_done)
    {
    screen_forcecls = TRUE;
    if (main_nowait) break;
    if (main_screensuspended)
      {
      int n;
      sys_mprintf(msgs_fid, "NE: ");
      if (fgets(cmd_buffer, cmd_buffer_size, kbd_fid) == NULL)
        cmd_buffer[0] = 0;
      n = strlen(cmd_buffer);
      if (n > 0 && cmd_buffer[n-1] == '\n') cmd_buffer[n-1] = 0;
      }
    else
      {
      scrn_rdline(FALSE, TRUE, "NE> ");
      s_move(0, 0);
      s_flush();
      }
    main_flush_interrupt();
    if (cmd_buffer[0] == 0 || cmd_buffer[0] == '\n') break;
    cmdline = cmd_buffer;
    }
  else break;
  }

if (main_screensuspended) scrn_restore();

if (!main_done)
  {
  if (!main_leave_message)
    {
    s_selwindow(message_window, -1, -1);
    s_cls();
    }
  main_pendnl = FALSE;
  ShowMark();
  }
}



/*************************************************
*         Handle read-only violation             *
*************************************************/

static void read_only_error(void)
{
error_moan(53);
scrn_rdline(FALSE, TRUE, "NE> ");
key_obey_commands(cmd_buffer);
screen_forcecls = TRUE;
scrn_display(FALSE);
}



/*************************************************
*         Handle one data keystroke              *
*************************************************/

void key_handle_data(int key)
{
char p = key;
int display_col = cursor_col;
BOOL waseof = (main_current->flags & lf_eof) != 0;

if (main_readonly) { read_only_error(); return; }

if (main_overstrike && cursor_col < main_current->len)
  {
  main_current->text[cursor_col] = key;
  cmd_recordchanged(main_current, cursor_col);
  }
else line_insertch(main_current, cursor_col, &p, 1, 0);


/* This is all rather dodgy ... but useful! */

if (main_binary)
  {
  int len = main_current->len;
  char *p = main_current->text;
  char *c, *h;

  while (len > 0 && isxdigit((unsigned char)(*p))) { p++; len--; }
  while (len > 0 && *p == ' ') { p++; len--; }

  h = p;

  while (len > 0 && (isxdigit((unsigned char)(*p)) || *p == ' ')) 
    { p++; len--; }

  if (len > 0 && *p == '*')
    {
    c = p + 2;
    p = main_current->text + cursor_col;

    while (c < main_current->text + main_current->len)
      {
      if (p == h || p == h+1)
        {
        int cc;
        int temp = tolower(h[0]);
        cc = ((isdigit(temp))? temp - '0' : temp - 'a' + 10) << 4;
        temp = tolower(h[1]);
        cc += (isdigit(temp))? temp - '0' : temp - 'a' + 10;
        *c = isprint(cc)? cc : '.';
        break;
        }

      if (p == c)
        {
        int temp = h[2];
        sprintf(h, "%2x", *c);
        h[2] = temp;
        display_col = h - main_current->text;
        break;
        }

      c++;
      h += 2;
      while (*h == ' ') h++;
      if (*h == '*') break;
      }
    }
  }


main_current->flags |= lf_shn;
if (cursor_col == main_rmargin) key_handle_function(s_f_lastchar);
else if (cursor_col < cursor_max)
  {
  scrn_displayline(main_current, cursor_row, waseof? cursor_offset : display_col);
  if (waseof && cursor_row < window_depth)
    scrn_displayline(main_bottom, cursor_row+1, cursor_offset);
  s_move(++cursor_col - cursor_offset, cursor_row);
  }
else key_handle_function(s_f_right);

ShowMark();        /* Might change indication of file changed */
s_selwindow(first_window, cursor_col - cursor_offset, cursor_row);
}



/*************************************************
*         Check scrolling possibility            *
*************************************************/

/* The yield is the number of lines that can be scrolled. */

static int tryscrollup(int amount)
{
int cando = 0;
linestr *top = window_vector[0];
while (amount--)
  {
  top = top->prev;
  if (top == NULL) break;
  cando++;
  }
return cando;
}


static int tryscrolldown(int amount)
{
int cando = 0;
linestr *bot = window_vector[window_depth];
if (bot != NULL) while (amount--)
  {
  bot = mac_nextline(bot);
  if (bot == NULL) break;
  cando++;
  }
return cando;
}



/*************************************************
*  Common code after horiz scroll/window change  *
*************************************************/

/* This can be called after lines have been deleted, in which case
the window_vector pointer is set to +1 to indicate "something unknown
is on the screen". */

void scrn_afterhscroll(void)
{
int i;
for (i = 0; i <= window_depth; i++)
  {
  linestr *line = window_vector[i];
  if (line != NULL && (int)line > 1) line->flags |= lf_shn;
  }
main_drawgraticules |= dg_both;
scrn_display(FALSE);
}



/*************************************************
*       Common code after vertical scroll        *
*************************************************/

static void aftervscroll(void)
{
int i;
main_current->flags |= lf_shn;
for (i = 0; i <= window_depth; i++) window_vector[i] = NULL;
window_vector[cursor_row] = main_current;
}



/*************************************************
*       Common cursor to true line start code    *
*************************************************/

static void do_csls()
{
cursor_col = 0;
if (cursor_offset > 0)
  {
  cursor_offset = 0;
  cursor_max = window_width;
  scrn_afterhscroll();
  }
}


/*************************************************
*           Adjust scroll so cursor shows        *
*************************************************/

/* This function is called after a cursor adjustment which
might require the window to be scrolled left or right. */

static void adjustscroll(void)
{
if (cursor_col < 0) cursor_col = 0;

if (cursor_col < cursor_offset)
  {
  while (cursor_col < cursor_offset)
    cursor_offset -= main_hscrollamount;
  cursor_max = cursor_offset + window_width;
  scrn_afterhscroll();
  }

else if (cursor_col > cursor_max)
  {
  while (cursor_col > cursor_max)
    {
    cursor_offset += main_hscrollamount;
    cursor_max = cursor_offset + window_width;
    }
  scrn_afterhscroll();
  }
}




/*************************************************
*      Common cursor hit window bottom code      *
*************************************************/

static void do_wbot()
{
int cando = tryscrolldown(main_vcursorscroll);
if (cando > window_depth) cando = window_depth;

if (cando != 0)
  {
  int i;
  linestr *nextline = (window_vector[window_depth] == NULL)?
    NULL : mac_nextline(window_vector[window_depth]);

  s_vscroll(window_depth, 0, -cando, FALSE);
  for (i = 0; i <= window_depth - cando; i++) window_vector[i] = window_vector[i+cando];

  for (i = window_depth - cando + 1; i <= window_depth; i++)
    {
    window_vector[i] = nextline;
    scrn_displayline(nextline, i, cursor_offset);
    if (nextline != NULL) nextline = nextline->next;
    }

   cursor_row = window_depth - cando + 1;
   main_current = main_current->next;
   main_currentlinenumber++;

  cmd_saveback();
  }
}



/*************************************************
*           Act on a group of lines              *
*************************************************/

/* This procedure is used for the Line Operations as
seen by the user, and also for some of the rectangular
operations. */

static void LineBlockOp(int op)
{
linestr *line = ((mark_type == mark_lines)||(op == lb_rectsp))? mark_line : main_current;
linestr *endline = main_current;

int above = line_checkabove(line);
int row = cursor_row - above;
int left = 0;     /* Otherwise compilers complain */
int right = 0;
int rectwidth = 0;

/* Set up left & right for rectangular operations */

if (op == lb_rectsp)
  {
  if (cursor_col < mark_col)
    {
    left = cursor_col;
    right = mark_col;
    }
  else
    {
    left = mark_col;
    right = cursor_col;
    }
  rectwidth =  right - left;
  }

/* Sort out top and bottom */

if (above < 0)
  {
  linestr *temp = line;
  line = main_current;
  endline = temp;
  row = cursor_row;
  }

/* Set up alignment point for align with previous */

if (op == lb_alignp)
  {
  linestr *prevline = line->prev;
  cursor_col = 0;
  if (prevline != NULL)
    {
    int i;
    char *p = prevline->text;
    for (i = 0; i < prevline->len; i++)
      if (p[i] != ' ') { cursor_col = i; break; }
    }
  }

/* Remove mark when required */

if (op == lb_rectsp ||
    (mark_type == mark_lines && (!mark_hold || op == lb_delete))
   ) CancelMark();

/* Deleting a block of lines is handled specially */

if (op == lb_delete)
  {
  int done = 1;
  int count = 0;

  while (done > 0)
    {
    if ((line->flags & lf_eof) != 0)
      {
      if (count == 0) return;          /* Not deleted anything */
      done = 0;
      }
    else
      {
      if (line == endline) done = -1;
      line = line_delete(line, TRUE);
      count++;
      }
    }

  main_current = line;
  if (above >= 0) main_currentlinenumber -= count + done;

  /* If running in a GUI window, we should be able just to scroll up
  the virual workspace. */

  #ifdef Windowing
  if (main_windowing)
    {
    sys_window_deletelines(main_currentlinenumber, count);
    cursor_row = main_currentlinenumber;
    return;
    }
  else
  #endif

  /* If at eof, allow complete re-format. Otherwise, if
  block was all on screen, scroll it up. */

  if (done == 0) screen_autoabove = FALSE; else
    {
    int botrow = cursor_row;
    int toprow = cursor_row - count + 1;
    if (above < 0)
      {
      botrow += count - 1;
      toprow = cursor_row;
      }
    if (botrow < window_depth && count <= botrow + 1)
      {
      int i;
      s_vscroll(window_depth, toprow, -count, TRUE);
      for (i = botrow+1; i <= window_depth; i++)
        window_vector[i-count] = window_vector[i];
      for (i = window_depth - count + 1; i <= window_depth; i++)
        window_vector[i] = NULL;
      }
    else     /* new current line in old place */
      {
      main_current->flags |= lf_shn;
      window_vector[cursor_row] = main_current;
      }
    }

  scrn_display(FALSE);
  }


/* The other operations can share code */

else for (;;)
  {
  BOOL longline = line->len > cursor_max + 1;
  BOOL onscreen = (0 <= row && row <= window_depth);
  int action;

  if ((line->flags & lf_eof) == 0) switch (op)
    {
    case lb_align: case lb_alignp:
      {
      #ifdef Windowing
      if (!main_windowing)
      #endif
        if (longline && onscreen)
          scrn_invertchars(line,row,cursor_max,1,FALSE);

      line_leftalign(line, cursor_col, &action);

      #ifdef Windowing
      if (main_windowing) sys_window_display_line(row, 0); else
      #endif

      if (onscreen)
        {
        if (cursor_offset > 0 || cursor_col > cursor_max ||
          (longline && action < 0))
          {
          line->flags |= lf_shn;
          scrn_display(FALSE);
          }
        else if (action > 0)
          {
          s_hscroll(0,row,window_width,row,action);
          if (line->len > cursor_max + 1)
            scrn_invertchars(line,row,cursor_max,1,TRUE);
          }
        else s_hscroll(0,row,window_width,row,action);
        }
      }
    break;

    case lb_eraseright:
    if (cursor_col < line->len)
      {
      line_deletech(line, cursor_col, line->len - cursor_col, TRUE);

      #ifdef Windowing
      if (main_windowing) sys_window_display_line(row, cursor_col); else
      #endif

      if (onscreen)
        {
        s_move(cursor_col - cursor_offset, row);
        s_eraseright();
        }
      }
    break;

    case lb_eraseleft:
    line_deletech(line, cursor_col, cursor_col, FALSE);

    #ifdef Windowing
    if (main_windowing) sys_window_display_line(row, 0); else
    #endif

    if (onscreen && cursor_col != cursor_offset)
      {
      if (cursor_offset > 0)
        {
        cursor_offset = 0;
        cursor_max = window_width + cursor_offset;
        scrn_afterhscroll();
        }
      else if (longline) line->flags |= lf_shn; else
        s_hscroll(0,row,window_width,row,cursor_offset-cursor_col);
      }
    break;

    case lb_closeup:
      {
      int i;
      int count = 0;
      char *p = line->text;

      for (i = cursor_col; i < line->len; i++)
        if (p[i] == ' ') count++; else break;
      line_deletech(line, cursor_col, count, TRUE);

      #ifdef Windowing
      if (main_windowing) sys_window_display_line(row, cursor_col); else
      #endif

      if (onscreen && count > 0)
        {
        if (longline) line->flags |= lf_shn; else
          s_hscroll(cursor_col-cursor_offset,row,window_width,row,-count);
        }
      }
    break;

    case lb_closeback:
      {
      int i;
      int count = 0;
      char *p = line->text;

      if (cursor_col > line->len) cursor_col = line->len;
      for (i = cursor_col - 1; i >= 0; i--)
        if (p[i] == ' ') count++; else break;
      line_deletech(line, cursor_col-count, count, TRUE);

      #ifdef Windowing
      if (main_windowing) sys_window_display_line(row, cursor_col-count); else
      #endif

      if (onscreen && count > 0)
        {
        if (longline || cursor_col - count < cursor_offset) line->flags |= lf_shn; else
          s_hscroll(cursor_col-cursor_offset-count,row,window_width,row,-count);
        }

      if (line == endline)
        {
        cursor_col -= count;
        if (cursor_col < cursor_offset)
          {
          while (cursor_col < cursor_offset)
            cursor_offset -= main_hscrollamount;
          cursor_max = window_width + cursor_offset;
          scrn_afterhscroll();
          }
        }
      }
    break;

    case lb_rectsp:
    line_insertch(line,left,0,0,rectwidth);

    #ifdef Windowing
    if (main_windowing) sys_window_display_line(row, left); else
    #endif

    if (onscreen) line->flags |= lf_shn;
    break;
    }

  row++;
  if (line == endline) break;
  line = line->next;
  }
}


/*************************************************
*        Handle one function keystroke           *
*************************************************/

/* The input value is either in the range 0-31, or is one of the values
defined in keyhdr.h for machine-independence.

The functions supplied by the terminal drivers are "actual
control keystrokes". Those with values < s_f_umax represent "ctrl"
type keystrokes and special keys. Those between s_f_umax and 200 are
"function" keystrokes. Both of these kinds are user settable, and
are translated into "logical control keystrokes via the vector
keytable. Keystrokes with values >= 200 are fixed in meaning and
cannot be changed by the user. We nevertheless translate via a
fixed table, to get them into the same space as the others. */

/* s_f_umax is the highest used special key function */
/* s_f_fbase is the base of fixed keys (= 200) */
/* s_f_fmax is the highest used fixed key */


void key_handle_function(int function)
{
if (function <= s_f_umax+max_fkey) function = key_table[function];
  else if (s_f_fbase <= function && function <= s_f_fmax)
    function = key_fixedtable[function-s_f_fbase];
  else function = ka_push;    /* unknown are ignored */

if (main_readonly && function >= ka_firstka && function <= ka_lastka &&
  !key_readonly[function - ka_firstka])
    { read_only_error();  return; }

/* Now process the function */

switch (function)
  {
  case ka_csl:                /* cursor left */
  if (cursor_col > cursor_offset) cursor_col--;
    else key_handle_function(s_f_left);
  break;

  case ka_csr:                /* cursor right */
  if (cursor_col - cursor_offset < screen_max_col) cursor_col++;
    else key_handle_function(s_f_right);
  break;

  case ka_cstab:              /* next tab */
  if (main_screentabs == NULL)
    {
    do { cursor_col++; }  while (cursor_col % 8 != 0);
    adjustscroll();
    }
  break;

  case ka_csptab:             /* previous tab */
  if (main_screentabs == NULL)
    {
    do { cursor_col--; }  while (cursor_col % 8 != 0);
    adjustscroll();
    }
  break;

  case ka_csu:                /* cursor up */
  if (cursor_row > 0)
    {
    cursor_row--;
    main_current = main_current->prev;
    main_currentlinenumber--;
    }
  else key_handle_function(s_f_top);
  break;

  case ka_csd:                /* cursor down */
  if (cursor_row < window_depth)
    {
    if (main_current->next != NULL)
      {
      cursor_row++;
      main_current = main_current->next;
      main_currentlinenumber++;
      }
    }
  else key_handle_function(s_f_bottom);
  break;

  case ka_cssl:               /* cursor to screen left */
  cursor_col = cursor_offset;
  break;

  case ka_cstl:               /* cursor to text left on screen */
    {
    char *p = main_current->text;
    int len = main_current->len;
    cursor_col = cursor_offset;
    while (cursor_col < len && p[cursor_col] == ' ' &&
      cursor_col < cursor_offset + window_width)
        cursor_col++;
    if (cursor_col >= len || p[cursor_col] == ' ')
      cursor_col = cursor_offset;
    }
  break;

  case ka_cstr:               /* cursor to text right on screen */
    {
    int len = main_current->len;
    if (len <= cursor_offset) cursor_col = cursor_offset;
    else
      {
      cursor_col = cursor_offset + window_width;
      if (cursor_col >= len) cursor_col = len;
      }
    }
  break;

  case ka_dc:                 /* delete character forwards */
  if (main_overstrike)
    {
    if (cursor_col < main_current->len)
      {
      main_current->text[cursor_col] = ' ';
      cmd_recordchanged(main_current, cursor_col);
      }
    }
  else line_deletech(main_current, cursor_col, 1, TRUE);
  scrn_displayline(main_current, cursor_row, cursor_col);
  break;

  case ka_dp:                 /* delete character backwards */
  if (cursor_col == 0) key_handle_function(s_f_leftdel); else
    {
    if (main_overstrike)
      {
      main_current->text[--cursor_col] = ' ';
      cmd_recordchanged(main_current, cursor_col);
      }
    else line_deletech(main_current, cursor_col--, 1, FALSE);
    if (cursor_col < cursor_offset) adjustscroll();
      else scrn_displayline(main_current, cursor_row, cursor_col);
    }
  break;

  case ka_csnl:               /* cursor to start of next line */
  if (cursor_row == window_depth) do_wbot(); else
    {
    linestr *next = mac_nextline(main_current);
    if (next != NULL)
      {
      main_current = next;
      main_currentlinenumber++;
      cursor_row++;
      }
    }
  do_csls();     /* move to true line start */
  break;

  case ka_cssbr:              /* cursor to screen bottom right */
  #ifdef Windowing
  if (main_windowing) sys_window_diag(FALSE); else
  #endif
    {
    int i = window_depth;
    main_current = window_vector[i];
    while (main_current == NULL) main_current = window_vector[--i];
    main_currentlinenumber += i - cursor_row;
    cursor_row = i;
    cursor_col = window_width + cursor_offset;
    }
  break;

  case ka_csstl:              /* cursor to screen top left */
  #ifdef Windowing
  if (main_windowing) sys_window_diag(TRUE); else
  #endif
    {
    main_current = window_vector[0];
    main_currentlinenumber -= cursor_row;
    cursor_col = cursor_offset;
    cursor_row = 0;
    }
  break;

  case ka_cswl:               /* cursor left by one word */
  for (;;)                    /* repeat for when backing up to previous line */
    {
    char *p = main_current->text;
    int len = main_current->len;
    if (cursor_col >= len) cursor_col = len;
    if (cursor_col > 0)
      {
      while (--cursor_col > 0 && (ch_tab[(unsigned int)(p[cursor_col])] & 
        ch_word) == 0);
      }
    if (cursor_col == 0)
      {
      if (main_current->prev == NULL) break;
      main_current = main_current->prev;
      main_currentlinenumber--;
      cursor_col = main_current->len;
      if (cursor_row > 0) cursor_row--; else scrn_display(FALSE);
      }
    else
      {
      while (cursor_col > 0 && (ch_tab[(unsigned int)(p[cursor_col])] & 
        ch_word) != 0) cursor_col--;
      if ((ch_tab[(unsigned int)(p[cursor_col])] & ch_word) == 0) cursor_col++;
      break;
      }
    }
  adjustscroll();
  break;

  case ka_cswr:               /* cursor right by one word */
    {
    BOOL first = TRUE;
    for (;;)                  /* loop for moving on to next line */
      {
      char *p = main_current->text;
      int len = main_current->len;
      if ((main_current->flags & lf_eof) != 0) break;
      if (first)
        while (cursor_col < len && (ch_tab[(unsigned int)(p[cursor_col])] & 
          ch_word) != 0) cursor_col++;
      while (cursor_col < len && (ch_tab[(unsigned int)(p[cursor_col])] & 
        ch_word) == 0) cursor_col++;
      first = FALSE;
      if (cursor_col >= len)
        {
        cursor_col = 0;
        main_current = mac_nextline(main_current);
        main_currentlinenumber++;
        scrn_display(FALSE);
        }
      else break;
      }
    }
  adjustscroll();
  break;

  case ka_csls:               /* cursor to true line start */
  do_csls();
  break;

  case ka_csle:               /* cursor to true line end */
  cursor_col = main_current->len;
  adjustscroll();
  break;

  case ka_split:    /* split line */
    {
    linestr *lineold = main_current;
    int i;
    int iline = window_depth - main_ilinevalue - 1;
    int row = cursor_row;

    main_current = line_split(main_current, cursor_col);
    main_currentlinenumber++;

    if (mark_line == lineold && mark_col >= cursor_col)
      {
      mark_line = main_current;
      mark_col -= cursor_col;
      }

    cursor_col = 0;
    if (main_AutoAlign)
      {
      int dummy;
      char *p = lineold->text;
      for (i = 0; i < lineold->len; i++)
        if (p[i] != ' ') { cursor_col = i; break; }
      if (cursor_col != 0) line_leftalign(main_current, cursor_col, &dummy);
      }

    #ifdef Windowing
    if (main_windowing)
      {
      sys_window_insertlines(main_currentlinenumber, 1);
      lineold->flags |= lf_clend;
      main_current->flags |= lf_shn;
      }
    else
    #endif
      {
      if (row > iline)
        for (i = 1; i <= row; i++) window_vector[i-1] = window_vector[i];
      else
        {
        for (i = window_depth; i > row + 2; i--)
          window_vector[i] = window_vector[i-1];
        cursor_row++;
        }

      window_vector[cursor_row] = main_current;

      if (cursor_offset == 0)
        {
        if (row > iline) s_vscroll(row, 0, -1, TRUE);
          else s_vscroll(window_depth, row+1, 1, TRUE);
        scrn_displayline(lineold, cursor_row - 1, cursor_col);
        scrn_displayline(main_current, cursor_row, 0);
        }
      else adjustscroll();
      }
    }
  break;


  case ka_last:  /* rightmost char typed - split the line */
    {
    linestr *lineold = main_current;
    int spacefound = FALSE;
    char *p = main_current->text;
    int i, sp;

    for (sp = cursor_col; sp > 0; sp--)
      if (p[sp] == ' ') { spacefound = TRUE; break; }

    s_move(cursor_col-cursor_offset, cursor_row);
    s_putc(p[cursor_col]);

    if (spacefound) cursor_col = sp + 1;

    if (cursor_col >= cursor_offset && cursor_col <= cursor_max)
      {
      s_move(cursor_col-cursor_offset, cursor_row);
      s_eraseright();
      }
    main_current = line_split(main_current, cursor_col);
    main_currentlinenumber++;

    cursor_col = main_rmargin - cursor_col + 1;

    if (main_AutoAlign)
      {
      int dummy;
      int indent = 0;
      char *p = lineold->text;
      for (i = 0; i < lineold->len; i++)
        if (p[i] != ' ') { indent = i; break; }
      if (indent)
        {
        line_leftalign(main_current, indent, &dummy);
        cursor_col += indent;
        }
      }

    #ifdef Windowing
    if (main_windowing)
      {
      sys_window_insertlines(main_currentlinenumber, 1);
      lineold->flags |= lf_clend;
      main_current->flags |= lf_shn;
      }
    else
    #endif
      {
      if (cursor_row > window_depth - 3)
        {
        for (i = 1; i <= cursor_row; i++) window_vector[i-1] = window_vector[i];
        s_vscroll(cursor_row, 0, -1, TRUE);
        }
      else
        {
        for (i = window_depth; i > cursor_row + 1; i--) window_vector[i] = window_vector[i-1];
        s_vscroll(window_depth, cursor_row++, 1, TRUE);
        }

      window_vector[cursor_row] = main_current;

      if (cursor_col >= cursor_offset && cursor_col <= cursor_max)
        {
        main_current->flags |= lf_shn;
        scrn_display(FALSE);
        }
      else adjustscroll();
      }
    }
  break;

  case ka_wleft:      /* hit left of screen window */
  if (cursor_offset > 0)
    {               /* part way into line */
    cursor_col--;
    adjustscroll();
    break;           /* that's all */
    }
  else               /* at true line start -- back up */
    {
    linestr *prev = main_current->prev;
    if (prev != NULL)
      {
      cursor_col = prev->len;
      if (cursor_col > cursor_max)
        cursor_offset = ((cursor_col/main_hscrollamount)-1)*main_hscrollamount;
      if (cursor_offset < 0) cursor_offset = 0;
      if (cursor_offset > 0)
        {
        cursor_max = cursor_offset + window_width;
        scrn_afterhscroll();
        }
      if (cursor_row != 0)
        {
        main_current = prev;
        main_currentlinenumber--;
        cursor_row--;
        break;       /* not at top of screen */
        }
      }
    }

  /* Fall through in to top of screen code. In windowing mode, we
  only get here if we hit the top of the file, since it is seen as
  one big virtual screen. In this case, do nothing. */

  case ka_wtop:       /* hit top of screen */
  #ifdef Windowing
  if (!main_windowing)
  #endif
    {
    int cando = tryscrollup(main_vcursorscroll);
    if (cando > window_depth) cando = window_depth;
    if (cando != 0)
      {
      int i;
      linestr *prev = main_current->prev;
      main_current = prev;
      main_currentlinenumber--;
      s_vscroll(window_depth, 0, cando, FALSE);
      for (i = window_depth; i >= cando; i--)
        window_vector[i] = window_vector[i-cando];
      for (i = cando-1; i >= 0; i--)
        {
        window_vector[i] = prev;
        scrn_displayline(prev, i, cursor_offset);
        prev = prev->prev;
        }
      cursor_row = cando - 1;
      cmd_saveback();
      }
    }
  break;

  case ka_wright:     /* hit right of window */
  cursor_col++;
  adjustscroll();
  break;

  case ka_wbot:      /* hit bottom of screen */
  do_wbot();
  break;

  case ka_reshow:    /* reshow screen */
  #ifdef Windowing
  if (main_windowing) sys_window_reshow(); else
  #endif
    {
    scrn_windows();
    s_selwindow(0, -1, -1);    /* select whole screen */
    s_cls();                   /* clear it */
    scrn_afterhscroll();
    }
  break;

  case ka_dpleft:   /* delete previous at left edge */
  /* If not scrolled left, delete a char and scroll left */
  if (cursor_offset > 0)
    {
    if ((main_current->flags & lf_eof) == 0)
      {
      line_deletech(main_current, cursor_col, 1, FALSE);
      cursor_col--;
      adjustscroll();
      break;
      }
    }

  /* Otherwise fall through into concatenate code */

  case ka_join:    /* concatenate with previous line */
  if (mark_type != mark_lines || mark_line != main_current)
    {
    linestr *prev = main_current->prev;
    if (prev != NULL)
      {
      main_currentlinenumber--;
      cursor_col = prev->len;
      if ((main_current->flags & lf_eof) != 0)
        {
        main_current = prev;
        cursor_row--;
        if (cursor_col == 0)          /* delete if previous empty line */
          LineBlockOp(lb_delete);
        adjustscroll();
        scrn_display(FALSE);
        }
      else
        {
        main_current = line_concat(main_current, 0);
        #ifdef Windowing
        if (main_windowing)
          {
          sys_window_deletelines(main_currentlinenumber + 1, 1);
          main_current->flags |= lf_shn;
          }
        else
        #endif
          {
          if (cursor_row != 0 && cursor_col >= cursor_offset && cursor_col < cursor_max)
            {
            int i;
            window_vector[cursor_row-1] = main_current;
            for (i = cursor_row; i < window_depth; i++)
              window_vector[i] = window_vector[i+1];
            window_vector[window_depth] = NULL;
            s_vscroll(window_depth, cursor_row, -1, TRUE);
            cursor_row--;
            scrn_displayline(main_current, cursor_row, cursor_col);
            }
          else adjustscroll();
          }
        }
      }
    }
  break;

  case ka_al:            /* align line with cursor */
  LineBlockOp(lb_align);
  break;

  case ka_alp:           /* align with previous line */
  LineBlockOp(lb_alignp);
  adjustscroll();
  break;

  case ka_cl:            /* close up line(s) */
  LineBlockOp(lb_closeup);
  break;

  case ka_clb:           /* close up lines(s) to the left */
  LineBlockOp(lb_closeback);
  break;

  /* Cut, copy, or delete an area of text or a rectangle. We
  remember the mark data, then delete the mark, then do the action. */

  case ka_cu:
  case ka_co:
  case ka_de:
  if (mark_type == mark_text || mark_type == mark_rect)
    {
    linestr *markline = mark_line;
    int markcol = mark_col;
    int type = mark_type;
    CancelMark();
    cut_cut(markline,markcol,type,function==ka_co,function==ka_de);
    adjustscroll();
    }
  break;

  case ka_dl:            /* delete line */
  LineBlockOp(lb_delete);
  break;

  case ka_dal:           /* erase left */
  LineBlockOp(lb_eraseleft);
  cursor_col = cursor_offset;
  break;

  case ka_dar:           /* erase right */
  LineBlockOp(lb_eraseright);
  break;

  case ka_dtwl:          /* delete to word left */
  e_dtwl(NULL);
  break;

  case ka_dtwr:          /* delete to word right */
  e_dtwr(NULL);
  break;

  case ka_lb:            /* mark/unmark line block */
  if (mark_type == mark_lines && !mark_hold) mark_hold = TRUE;
    else SetMark(mark_lines);
  break;

  case ka_gm:            /* mark/unmark global limit */
  SetMark(mark_global);
  break;

  /* Paste data from the cut buffer back into the file; it
  is either a string of text or a rectangle. Keep the current
  line where it is on the screen, except when the total file
  is small, or the pasting is big and near the top of the screen. */

  case ka_pa:
  if (cut_buffer != NULL)
    {
    if (cut_type == cuttype_text)
      {
      int row = cursor_row;
      #ifdef Windowing
      if (main_windowing)
        {
        int d[4];
        sys_window_info(d);
        row -= d[3];
        }
      #endif
      if ((cut_pastetext() > row && row < 10) ||
        main_linecount < window_depth) screen_autoabove = FALSE;
      }
    else cut_pasterect();
    }
  adjustscroll();
  break;

  case ka_rb:            /* start rectangular operation */
  SetMark(mark_rect);
  break;


  case ka_rc:            /* read commands */
  #ifdef Windowing
  if (main_windowing)
    {
    sys_display_cursor(TRUE);
    scrn_rdline(FALSE, FALSE, "Enter NE command line");
    sys_display_cursor(FALSE);
    }
  else
  #endif
    {
    scrn_invertchars(main_current, cursor_row, cursor_col, 1, TRUE);
    scrn_rdline(FALSE, TRUE, "NE> ");
    s_selwindow(first_window, -1, -1);
    scrn_invertchars(main_current, cursor_row, cursor_col, 1, FALSE);
    s_selwindow(message_window, 0, 0);
    s_flush();
    }
  key_obey_commands(cmd_buffer);
  break;

  case ka_rs:            /* insert rectangle of spaces */
  if (mark_type == mark_rect) LineBlockOp(lb_rectsp);
  break;

  case ka_tb:            /* start text string operation */
  SetMark(mark_text);
  break;

  case ka_scleft:        /* scroll left */
  #ifdef Windowing
  if (main_windowing) sys_window_scroll(wscroll_left); else
  #endif

  if (cursor_offset > 0)
    {
    cursor_offset -= main_hscrollamount;
    if (cursor_offset < 0) cursor_offset = 0;
    cursor_max = window_width + cursor_offset;
    if (cursor_col > cursor_max) cursor_col = cursor_max;
    scrn_afterhscroll();
    }
  else cursor_col = 0;
  break;

  case ka_scright:    /* scroll right */
  #ifdef Windowing
  if (main_windowing) sys_window_scroll(wscroll_right); else
  #endif
    {
    cursor_offset += main_hscrollamount;
    cursor_max = cursor_offset + window_width;
    if (cursor_col < cursor_offset) cursor_col = cursor_offset;
    scrn_afterhscroll();
    }
  break;

  case ka_scup:         /* scroll up */
  #ifdef Windowing
  if (main_windowing) sys_window_scroll(wscroll_up); else
  #endif
    {
    int cando = tryscrollup(window_depth);

    /* No upward scrolling possible. Move current to top line. */

    if (cando == 0)
      {
      main_current = main_top;
      main_currentlinenumber = 0;
      }

    /* Scroll up by screen - 2 lines, or until hit the top. Move the
    current line back by one scroll amount if possible. */

    else
      {
      int i;
      int oldcurrentlinenumber = main_currentlinenumber;
      linestr *oldcurrent = main_current;
      linestr *top = window_vector[0];
      BOOL hittop = FALSE;
      BOOL changecurrent = TRUE;

      /* Don't change current if it will be visible on new, fully-scrolled
      screen. */

      if (cursor_row <= 1)
        {
        cursor_row += window_depth - 1;
        changecurrent = FALSE;
        }

      /* Find new top line, moving current back too, if required. */

      for (i = 1; i < window_depth; i++)
        {
        linestr *prev = top->prev;
        if (prev == NULL) hittop = TRUE; else top = prev;
        if (changecurrent && main_current->prev != NULL)
          {
          main_current = main_current->prev;
          main_currentlinenumber--;
          }
        }

      /* If hit the top, force it to be the top displayed line. If the
      previous current is still visible, re-instate it. */

      if (hittop)
        {
        scrn_hint(sh_topline, 0, top);
        for (i = 1; i <= window_depth && top != NULL; i++)
          {
          top = top->next;
          if (top == oldcurrent)
            {
            main_current = oldcurrent;
            main_currentlinenumber = oldcurrentlinenumber;
            break;
            }
          }
        }
      else aftervscroll();
      }

    scrn_display(FALSE);
    }
  cmd_saveback();
  break;

  case ka_scdown:        /* scroll down */
  #ifdef Windowing
  if (main_windowing) sys_window_scroll(wscroll_down); else
  #endif
    {
    int cando = tryscrolldown(window_depth);

    /* No downward scrolling possible. Move current to bottom line. */

    if (cando == 0)
      {
      main_current = main_bottom;
      main_currentlinenumber = main_linecount - 1;
      screen_autoabove = FALSE;
      }

    /* Scroll down by screen - 2 lines, or until hit the bottom. Move the
    current line forward by one scroll amount if possible. */

    else
      {
      int i;
      int hitbot = FALSE;
      int changecurrent = TRUE;
      int oldcurrentlinenumber = main_currentlinenumber;
      linestr *oldcurrent = main_current;
      linestr *bot = window_vector[window_depth];

      /* Don't change current if it will be visible on new, fully-scrolled
      screen. */

      if (cursor_row >= window_depth-1)
        {
        cursor_row -= (cando == window_depth)? cando-1 : cando;
        changecurrent = FALSE;
        }

      /* Find new bottom line, moving current forwards too, if required. */

      for (i = 1; i < window_depth; i++)
        {
        if ((bot->flags & lf_eof) != 0) hitbot = TRUE; else bot = mac_nextline(bot);
        if (changecurrent && main_current->next != NULL)
          {
          main_current = main_current->next;
          main_currentlinenumber++;
          }
        }

      /* If hit the bottom, force it to be the bottom displayed line. If the
      previous current is still visible, re-instate it. */

      if (hitbot)
        {
        linestr *top = bot;
        for (i = 1; i <= window_depth; i++)
          {
          if (top->prev == NULL) break;
          top = top->prev;
          if (top == oldcurrent)
            {
            main_current = oldcurrent;
            main_currentlinenumber = oldcurrentlinenumber;
            }
          }
        scrn_hint(sh_topline, 0, top);
        }
      else aftervscroll();
      }

    scrn_display(FALSE);
    }
  cmd_saveback();
  break;

  case ka_sctop:         /* scroll top */
  main_current = main_top;
  main_currentlinenumber = 0;
  scrn_display(FALSE);
  cmd_saveback();
  break;

  case ka_scbot:         /* scroll bottom */
  while (file_extend() != NULL && !main_interrupted(ci_move));
  main_current = main_bottom;
  main_currentlinenumber = main_linecount - 1;
  scrn_display(FALSE);
  cmd_saveback();
  break;

  /* The definitions of the ka_xxx keystrokes are arranged so
  that their values are all greater than the maximum function
  keystring. Hence we can now just check for a number in the
  relevant range and take it to be a string. Negative numbers
  (modulo 255) represent calls to the invariant keystrings. */

  default:
  if ((1 <= function && function <= max_keystring) ||
     (256 - max_fixedKstring <= function && function <= 255))
    {
    char *keydata = (function < 256 - max_fixedKstring)?
      main_keystrings[function] : main_fixedKstrings[256 - function];

    if (keydata != NULL)
      {
      #ifdef Windowing
      if (!main_windowing)
      #endif

        {
        s_selwindow(message_window, 0, 0);
        s_cls();
        s_printf("NE> %s", keydata);
        s_move(0, 0);    /* indicate being obeyed */
        s_flush();       /* flush buffered output */
        }

      key_obey_commands(keydata);
      if (!main_done && currentbuffer != NULL) scrn_display(FALSE);
      }
    }
  break;
  }

if (!main_done && currentbuffer != NULL)
  {
  if (currentbuffer->to_fid != NULL) main_checkstream();
  scrn_display(FALSE);
  ShowMark();
  s_selwindow(first_window, cursor_col - cursor_offset, cursor_row);
  }
}

/* End of ekey.c */
