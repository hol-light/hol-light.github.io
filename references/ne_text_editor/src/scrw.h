/*************************************************
*                      SCRW                      *
*************************************************/

/* SCRW is a set of routines for displaying text in a scrolling window.
It has some things in common with Acorn's txt routines, but is simpler
and more light-weight. It uses the xs library of store handling functions,
which should be compiled as part of the application. */

/* This is the header file which defines the SCRW interface. */

/* Philip Hazel, November 1993 */
/* Last modified: November 1993 */

/* Flags for store types */

#define scrw_xs_only          0
#define scrw_malloc_only      1
#define scrw_xs_then_malloc   2
#define scrw_malloc_then_xs   3

/* Renditions */

#define rend_inverse     0x01
#define rend_underscore  0x02
#define rend_bold        0x04
#define rend_blink       0x08
#define rend_all         (rend_inverse | rend_underscore | rend_bold | rend_blink)
#define rend_default     0x20

/* Functions */

extern int  scrw_delete(int);
extern int  scrw_empty(int, int);
extern int  scrw_init(window_data *, int, int, int, int);
extern void scrw_poll(int, int *);
extern void scrw_puts(int, char *);

/* End of scrw.h */
