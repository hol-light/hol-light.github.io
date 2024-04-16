/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1993 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: September 1994 */

/* This file contains code for miscellaneous screen-handling subroutines */


#include "ehdr.h"
#include "shdr.h"


/*************************************************
*                Initialize                      *
*************************************************/

void scrn_init(BOOL changemargin)
{
int i;

window_top = 1;
window_bottom = screen_max_row - 2;
window_width = screen_max_col;
window_depth = window_bottom - window_top;
main_drawgraticules = dg_both;
cursor_max = cursor_offset + window_width;
if (changemargin) main_rmargin = window_width;

window_vector = store_Xget((screen_max_row+1)*sizeof(linestr *));
for (i = 0; i <= window_depth; i++) window_vector[i] = NULL;
}



/*************************************************
*                Terminate                       *
*************************************************/

void scrn_terminate(void)
{
store_free(window_vector);
window_vector = NULL;
}


/*************************************************
*           Define relevant windows              *
*************************************************/

void scrn_windows(void)
{
s_defwindow(message_window, screen_max_row, screen_max_row);
s_defwindow(first_window, window_bottom, window_top);
s_defwindow(first_window+1, window_bottom+1, window_bottom+1);
}



/*************************************************
*              Change size                       *
*************************************************/

/* New values will be in screen_max_row and screen_max_col */

void scrn_setsize(void)
{
linestr *topline = window_vector[0];

scrn_terminate();
s_terminate();
s_init(screen_max_row, screen_max_col, TRUE);
scrn_init(FALSE);
scrn_windows();

if (cursor_col > cursor_max) cursor_col = cursor_offset + window_width/2;

if (cursor_row >= window_depth)
  {
  while (cursor_row > window_depth/2)
    {
    cursor_row--;
    main_current = main_current->prev;
    main_currentlinenumber--;
    }
  }

scrn_hint(sh_topline, 0, topline);
scrn_display(FALSE);
}


/*************************************************
*                   Suspend                      *
*************************************************/

void scrn_suspend(void)
{
scrn_terminate();
if (sys_resetterminal != NULL) sys_resetterminal();
main_screensuspended = TRUE;
main_screenOK = FALSE;
}


/*************************************************
*                   Restore                      *
*************************************************/

void scrn_restore(void)
{
if (sys_setupterminal != NULL) sys_setupterminal();
scrn_init(FALSE);
scrn_windows();
screen_forcecls = TRUE;
main_screensuspended = FALSE;
main_screenOK = TRUE;
}

/* End of escrnsub.c */
