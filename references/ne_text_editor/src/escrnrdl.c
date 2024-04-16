/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 2003 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: March 2003 */


/* This file contains code for reading a command line off the screen, when
in non-wimp screen mode. */


#include "ehdr.h"
#include "keyhdr.h"
#include "shdr.h"
#include "scomhdr.h"


static int promptlen;
static int scrolled;
static int pmax;



/*************************************************
*              Reshow altered line               *
*************************************************/

static void reshow(int p, BOOL changed, int showoff, char *prompt)
{
int i;
if (p + promptlen < scrolled)
  {
  if (p == 0)
    {
    scrolled = 0;
    s_cls();
    s_printf("%s", prompt);
    for (i = 0; i < pmax && i + promptlen <= window_width; i++)
      s_putc(cmd_buffer[i]);
    }
  else while (p + promptlen < scrolled)
    {
    s_hscroll(0, 0, window_width, 0, 1);
    s_move(0, 0);
    s_putc(cmd_buffer[--scrolled - promptlen]);
    }
  }

else if (changed)
  {
  s_move(p + promptlen - scrolled + showoff, 0);
  for (i = p + showoff; i < pmax && i + promptlen - scrolled <= window_width; i++)
    s_putc(cmd_buffer[i]);
  if (i + promptlen - scrolled < window_width) s_eraseright();
  }

while (p + promptlen - scrolled > window_width)
  {
  int pp = (++scrolled) + window_width - promptlen;
  s_hscroll(0, 0, window_width, 0, -1);
  if (pp < pmax)
    {
    s_move(window_width, 0);
    s_putc(cmd_buffer[pp]);
    }
  }
}



/*************************************************
*             Read Command Line                  *
*************************************************/

void scrn_rdline(BOOL escape_flag, BOOL stack_flag, char *prompt)
{
int interactend = FALSE;
int p = 0;
int sp = cmd_stackptr;

#ifdef Windowing
if (main_windowing)
  {
  sys_window_cmdline(prompt);
  return;
  }
#endif

main_rc = error_count = 0;  /* in case 40 errors without return to top level! */

if (sp == 0) sp = -1;
pmax = 0;
scrolled = 0;

if (main_pendnl) sys_mprintf(msgs_fid, "\r\n"); else
  {
  s_selwindow(message_window, 0, 0);
  s_cls();
  }

s_move(0, 0);
s_printf("%s", prompt);
promptlen = s_x();

main_flush_interrupt();

while (!interactend)
  {
  int type;
  int key = sys_cmdkeystroke(&type);

  /* An interrupt terminates, leaving the flag set */

  if (main_escape_pressed)
    {
    cmd_buffer[pmax] = 0;
    return;
    }

  /* A data key causes a character to be added to the command line, which
  may have to scroll left. */

  if (type == ktype_data)
    {
    int i;
    if (p > pmax)
      {
      for (i = pmax; i < p; i++) cmd_buffer[i] = ' ';
      pmax = p;
      }
    for (i = pmax-1; i >= p; i--) cmd_buffer[i+1] = cmd_buffer[i];
    pmax++;
    cmd_buffer[p++] = key;
    reshow(p, TRUE, -1, prompt);
    }

  /* A function key is translated in the same fashion as normal, then
  handled specially, except for the RETURN key, which is always taken
  as "end of command". */

  else
    {
    if (key == '\r') key = ka_ret;
      else if (key <= s_f_umax+max_fkey) key = key_table[key];
        else if (s_f_fbase <= key && key <= s_f_fmax)
          key = key_fixedtable[key-s_f_fbase];
        else key = ka_push;      /* unknown are ignored */

    switch (key)
      {
      case ka_ret:               /* RETURN pressed */
      cmd_buffer[pmax] = 0;
      interactend = TRUE;
      break;

      case ka_csu:               /* Cursor up */
      if (stack_flag)
        {
        if (sp == 0) sp = cmd_stackptr;
        if (sp > 0)
          {
          strcpy(cmd_buffer, cmd_stack[--sp]);
          p = pmax = strlen(cmd_buffer);
          if (p > window_width - promptlen)
            {
            scrolled = p - window_width/2;
            s_cls();
            reshow(scrolled - promptlen, TRUE, 0, prompt);
            }
          else
            {
            scrolled = BIGNUMBER;
            reshow(0, TRUE, 0, prompt);
            }
          }
        }
      break;

      case ka_csd:               /* Cursor down */
      if (stack_flag)
        {
        if (sp >= 0)
          {
          if (++sp >= cmd_stackptr) sp = 0;
          strcpy(cmd_buffer, cmd_stack[sp]);
          p = pmax = strlen(cmd_buffer);
          if (p > window_width - promptlen)
            {
            scrolled = p - window_width/2;
            s_cls();
            reshow(scrolled - promptlen, TRUE, 0, prompt);
            }
          else
            {
            scrolled = BIGNUMBER;
            reshow(0, TRUE, 0, prompt);
            }
          }
        }
      break;

      case ka_csl:               /* Cursor left */
      if (p > 0) reshow(--p, FALSE, 0, prompt);  else if (scrolled)
        {
        scrolled = promptlen + 1;
        reshow(p, FALSE, 0, prompt);
        }
      break;

      case ka_cswl:             /* Cursor word left */
      if (p > 0)
        {
        while (--p > 0 && 
          (ch_tab[(unsigned char)(cmd_buffer[p])] & ch_word) == 0);
        while (p > 0 && 
          (ch_tab[(unsigned char)(cmd_buffer[p])] & ch_word) != 0) p--;
        if ((ch_tab[(unsigned char)(cmd_buffer[p])] & ch_word) == 0) p++;
        }
      else if (scrolled) scrolled = promptlen + 1;
      reshow(p, FALSE, 0, prompt);
      break;

      case ka_cstl:             /* Cursor text left */
      p = (scrolled < promptlen)? 0 : (scrolled - promptlen);
      break;

      case ka_cstr:             /* Cursor text right */
      p = scrolled + window_width - promptlen;
      if (p > pmax) p = pmax;
      break;

      case ka_csls:             /* cursor to true line start */
      p = 0;
      scrolled = promptlen+1;
      reshow(p, FALSE, 0, prompt);
      break;

      case ka_csle:             /* cursor to true line end */
      p = pmax;
      reshow(p, FALSE, 0, prompt);
      break;

      case ka_csr:              /* Cursor right */
      reshow(++p, FALSE, 0, prompt);
      break;

      case ka_cswr:             /* cursor word right */
      while (p < pmax && 
        (ch_tab[(unsigned char)(cmd_buffer[p])] & ch_word) != 0) p++;
      while (p < pmax && 
        (ch_tab[(unsigned char)(cmd_buffer[p])] & ch_word) == 0) p++;
      reshow(p, FALSE, 0, prompt);
      break;

      /* Tab used to move the pointer (but didn't always work)*/
#ifdef NEVER       
      case ka_cstab:            /* next tab */
      if (main_screentabs == NULL)
        {
        do {p++; }  while (p % 8 != 0);
        if (p > pmax) p = pmax;
        reshow(p, FALSE, 0, prompt);
        }
      break;
#endif

      /* Tab is not used for file name completion */
      case ka_cstab:
        {
        int oldp = p;  
        p = sys_fcomplete(cmd_buffer, p, &pmax);
        reshow(oldp, TRUE, 0, prompt);
        } 
      break;          

      case ka_csptab:             /* previous tab */
      if (main_screentabs == NULL)
        {
        do { p--; }  while (p % 8 != 0);
        if (p <= 0) { p = 0; scrolled = promptlen + 1; }
        reshow(p, FALSE, 0, prompt);
        }
      break;

      case ka_dp:               /* Delete previous */
      if (p > 0)
        {
        int i;
        for (i = p; i < pmax; i++) cmd_buffer[i-1] = cmd_buffer[i];
        pmax--;
        if (p + promptlen == scrolled && scrolled > 0) scrolled--;
        reshow(--p, TRUE, 0, prompt);
        }
      else if (scrolled)
        {
        scrolled = promptlen + 1;
        reshow(p, FALSE, 0, prompt);
        }
      break;

      case ka_dc:               /* Delete current */
        {
        int i;
        for (i = p; i < pmax - 1; i++) cmd_buffer[i] = cmd_buffer[i+1];
        pmax--;
        reshow(p, TRUE, 0, prompt);
        }
      break;

      case ka_dar:              /* Delete all right */
      pmax = p;
      s_eraseright();
      break;

      
      case ka_dtwl:             /* Delete to word left */
        { 
        int pp = p; 
        if (p >= pmax) p = pmax;
        while (--p > 0 && 
          (ch_tab[(unsigned char)(cmd_buffer[p])] & ch_word) == 0);
        while (p > 0 && 
          (ch_tab[(unsigned char)(cmd_buffer[p])] & ch_word) != 0) p--;
        if ((ch_tab[(unsigned char)(cmd_buffer[p])] & ch_word) == 0) p++; 
        if (pp > p)
          {
          int i;
          int count = pmax - pp;
          for (i = 0; i < count; i++) cmd_buffer[p+i] = cmd_buffer[pp+i];
          pmax -= (pp - p); 
          if (scrolled > 0 && p + promptlen < scrolled) 
            {
            if (p == 0) scrolled = promptlen + 1;   
              else scrolled -= scrolled - (p + promptlen);  
            } 
          reshow(p, TRUE, 0, prompt);
          }
        }
      break;  
       
      case ka_dtwr:             /* Delete to word right */
        {
        int pp = p;
        if (pp >= pmax) pp = pmax;
        while (pp < pmax && 
          (ch_tab[(unsigned char)(cmd_buffer[pp])] & ch_word) != 0) pp++;
        while (pp < pmax && 
          (ch_tab[(unsigned char)(cmd_buffer[pp])] & ch_word) == 0) pp++;
        if (pp > p)
          {
          int i;
          int count = pmax - pp;
          for (i = 0; i < count; i++) cmd_buffer[p+i] = cmd_buffer[pp+i];
          pmax -= (pp - p); 
          reshow(p, TRUE, 0, prompt);
          }
        }
      break;

      case ka_dal:              /* Delete all left */
        {
        int i;
        int j = 0;
        s_cls();
        s_printf("%s", prompt);
        for (i = p; i < pmax; i++) cmd_buffer[j++] = cmd_buffer[i];
        pmax = j;
        p = scrolled = 0;
        reshow(p, TRUE, 0, prompt);
        }
      break;

      case ka_dl:               /* Delete line */
      s_cls();
      s_printf("%s", prompt);
      p = pmax = scrolled = 0;
      reshow(p, TRUE, 0, prompt);
      break;

      default:        /* Deal with the use of function keystrings */
      if ((1 <= key  && key <= max_keystring) ||
         (256 - max_fixedKstring <= key && key <= 255))
        {
        char *keydata = (key < 256 - max_fixedKstring)?
          main_keystrings[key] : main_fixedKstrings[256 - key];

        if (keydata != NULL)
          {
          s_cls();
          s_printf("E> %s", keydata);
          strcpy(cmd_buffer, keydata);
          interactend = TRUE;
          }
        }
      break;
      }
    }

  /* Always make cursor correct before looping */

  s_move(p + promptlen - scrolled, 0);
  }

/* Cursor is on a printing line */

main_pendnl = TRUE;
}

/* End of escrnrdl.c */
