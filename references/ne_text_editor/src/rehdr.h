/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991, 1992, 1993 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: September 1994 */

/* Header file for regular expression handling */

/* #define REDEBUG */
/* #define REDEBUGW */
/* #define REDEBUGWF */

/* These procedures implement the matching of Regular Expressions on
lines of text. There are three entries from the outside world: makeFSM
to compile a qualified string into a finite state machine; matchqsR, to
match a qualified string with the R qualifier to a given line (if
necessary the expression is re-compiled before matching takes place); and
ReChange, which makes a change to a line after it has been matched with
a regular expression, possibly making use of the "wild strings".

The syntax of regular expressions is as follows:

<exp>  := [<prim>*] ['|' [<prim>*]]   (at least one <prim> must exist)

<prim> := (<exp>) | #<prim> | ^<prim> | <chm>

<chm>  := ? | <pchm> | <nchm>

<pchm> := <ch> | <ch>-<ch> | $<code> | $H<hex> | $H<hex>-<hex>

<nchm> := !<pchm> | !(<pchm> ['|' <pchm>]*)

<code> := A | a | D | d | L | l | P | p | W | w

<ch>   := "<any character> | <any non-metacharacter>   in non-hex mode
       OR
<ch>   := <hex>                                        in hex mode

<hex>  := <any two hex digits, in either case>

The metacharacters are: ( ) | ? # ^ $ - ! "

Regular expressions are handled using an adaption of an algorithm by
Martin Richards. The main adaption is to make it run backwards (from
right to left) for the benefit of the E and L qualifiers and for
backwards searches. There are also extensions for null alternatives,
"one or more occurrences of", and negated alternation groups where
each alternative is a single character.

The expression is compiled into the definition of a finite state
machine. The matching process then runs this machine, maintaining a
number of simultaneously active states (essentially, many
simultaneously active machines).

In order to identify the "wild" strings in the matched portion of
the line, a journal can optionally be maintained during the matching
process. (This is another extension to the Richards algorithm.) A
journal vector, with its length in the first word, is passed by the
called of matchqsR. If the match is a success, this vector, on return,
contains a sequence of BCPL strings, packed end to end.

The data for the machine is held in four byte vectors, whose addresses
are placed in globals during processing, for simplicity of coding and
efficiency. They are:

R_String: The characters of the expression, with the length in byte
          zero, reversed if working backwards. The state numbers for
          the fsm correspond to the offsets in this vector. Initially
          state 1 (the first char in the string) is active.

R_States: When a match succeeds for character <n> then the state
          whose number is in R_States%<n> becomes active.

R_Sflags: One bit (R_Matchstate) is set in this byte if the corres-
          ponding state requires a character to be matched. Another
          bit (R_Listed) is set during matching if the state has
          already been put on the active state list (saves a bit of
          searching). A third bit (R_Endalt) is set for states that
          are the end of an alternation at the outermost level. This
          is used to determine the ends of wild strings that are
          being matched.

R_List:   This vector is used to hold the list of currently active
          states during matching.

The global variable R_ListPtr is used as the high water mark of
R_List while matching. It is also used to keep track of the current
position in the string while compiling. */


#define JournalSize   4096

#define R_MatchState  0x80
#define R_Listed      0x40
#define R_XListed     0xBF
#define R_Endalt      0x20


#define mac_hex(ms, ls) \
  (((isdigit(ms)? (ms) - '0' : tolower(ms) - 'a' + 10) << 4) + \
    (isdigit(ls)? (ls) - '0' : tolower(ls) - 'a' + 10))


/* Local Globals (if you see what I mean). The R_Journal variable
is more global, because it is set up by commands that want it. */

extern unsigned char *R_String;
extern unsigned char *R_States;
extern unsigned char *R_Sflags;
extern unsigned char *R_List;

extern int R_ListPtr;
extern int R_Bracount;
extern int R_Jptr;

/* End of regexphdr.h */
