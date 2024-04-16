/*************************************************
*       The E text editor - 2nd incarnation      *
*************************************************/

/* Copyright (c) University of Cambridge, 1991 - 1999 */

/* Written by Philip Hazel, starting November 1991 */
/* This file last modified: July 1999 */


/* This file contains code for handling input and output */

#include "ehdr.h"

#define buffgetsize 1024


/*************************************************
*          Support routines for backups          *
*************************************************/

BOOL file_written(char *name)
{
filewritstr *p = files_written;
while (p != NULL)
  {
  #ifdef FILE_CASELESS
  char *s = name;
  char *t = p->name;  
  while ((*t || *s) && tolower(*t) == tolower(*s)) { t++; s++; }
  if (*t == 0 && *s == 0) return TRUE; 
  #else  
  if (strcmp(name, p->name) == 0) return TRUE;
  #endif 
  p = p->next; 
  }
return FALSE;
}     

void file_setwritten(char *name)
{
filewritstr *p;
if (file_written(name)) return;
p = store_Xget(sizeof(filewritstr));
p->name = store_copystring(name);
p->next = files_written;
files_written = p;
}



/*************************************************
*      Get next input line and binarize it       *
*************************************************/

static linestr *file_nextbinline(FILE **f, int *binoffset)
{
FILE *ff = *f;
linestr *line = store_getlbuff(80);
int c = EOF;

if (ff != NULL)
  {
  int i;
  char *s = line->text;
  char *p = s + 8;
  char cc[17];
  cc[16] = 0;

  sprintf(s, "%06x  ", *binoffset);
  *binoffset += 16;

  for (i = 0; i < 16; i++)
    {
    c = fgetc(ff);
    if (c == EOF)
      {
      if (i == 0) { line->len = 0; goto READ; }
      sprintf(p, "   ");
      }
    else sprintf(p, "%02x ", c);
    p += 3;
    if (i == 7) *p++ = ' ';
    cc[i] = isprint(c)? c : '.';
    }
  sprintf(p, " * %s *", cc);
  line->len = (p - s + 21);
  }
else line->len = 0;

READ:

if (c == EOF)
  {
  if (line->len == 0) line->flags |= lf_eof;
  if (ff != NULL)
    {
    fclose(ff);
    *f = NULL;
    }
  }

return line;
}



/*************************************************
*            Get next input line                 *
*************************************************/

/* Returns an eof buffer at end of file. We get a buffer of large(ish) size,
and free off the end of it if possible. When a line is very long, we have
to copy it into a longer buffer. The binoffset variable is non-null when we
want to read a "binary line". The buffer variable is non-null when we want to
check the buffer for being over-length if it is a stream. */

linestr *file_nextline(FILE **f, int *binoffset, bufferstr *buffer)
{
BOOL eof = FALSE;
BOOL tabbed = FALSE;
FILE *ff;
int length, maxlength;
linestr *line;
char *s;

if (main_binary && binoffset != NULL)
  line = file_nextbinline(f, binoffset);
else
  {
  ff = *f;
  length = 0;
  maxlength = buffgetsize;
  line = store_getlbuff(buffgetsize);
  s = line->text;

  if (ff == NULL) eof = TRUE; else while (length < buffgetsize)
    {
    int c = fgetc(ff);
    if (c == '\n') break;
    if (c == EOF)
      {
      if (length == 0 || ferror(ff)) eof = TRUE;
      break;
      }
    if (c == '\t' && main_tabin)
      {
      tabbed = main_tabflag;
      do
        {
        *s++ = ' ';
        length++;
        if (length >= buffgetsize) break;
        }
      while ((length % 8) != 0);
      }
    else { *s++ = c; length++; }
    }

  line->len = length;

  /* Deal with lines longer than the buffer size. If we run out of store,
  the line gets chopped. */

  while (length >= maxlength)
    {
    char *newtext = store_get(length+buffgetsize);
    if (newtext == NULL) break;
    memcpy(newtext, line->text, length);
    store_free(line->text);
    line->text = newtext;
    s = line->text + length;
    maxlength += buffgetsize;

    while (length < maxlength)
      {
      int c = fgetc(ff);
      if (c == '\n' || c == EOF) break;
      if (c == '\t' && main_tabin)
        {
        tabbed = main_tabflag;
        do
          {
          *s++ = ' ';
          length++;
          if (length >= maxlength) break;
          }
        while ((length % 8) != 0);
        }
      else { *s++ = c; length++; }
      }

    line->len = length;
    }

  /* Free up unwanted store at end of buffer, or free the whole buffer if
  this is an empty line. */

  if (length > 0) store_chop(line->text, length); else
    {
    store_free(line->text);
    line->text = NULL;
    }

  /* At end of file, close input */

  if (eof)
    {
    line->flags |= lf_eof;
    if (ff != NULL)
      {
      fclose(ff);
      *f = NULL;
      }
    }

  if (tabbed) line->flags |= lf_tabs;
  }

if (buffer != NULL && buffer == currentbuffer && buffer->to_fid != NULL) 
  main_checkstream();

return line;
}



/*************************************************
*          Extend chain of lines                 *
*************************************************/

/* If not at the end of the file, read another line and chain
it onto the queue of lines, returning the new line. Else return NULL. */

linestr *file_extend(void)
{
linestr *last = main_bottom;
if ((main_bottom->flags & lf_eof) != 0) return NULL;
main_bottom = file_nextline(&from_fid, &currentbuffer->binoffset, currentbuffer);
main_bottom->key = ++main_imax;
last->next = main_bottom;
main_bottom->prev = last;
main_linecount++;
return main_bottom;
}


/*************************************************
*           Write a line's characters            *
*************************************************/

int file_writeline(linestr *line, FILE *f)
{
int i;
int len = line->len;
char *p = line->text;

/* Handle binary output */

if (main_binary)
  {
  BOOL ok = TRUE; 
  while (len > 0 && isxdigit((unsigned int)(*p))) { len--; p++; }

  while (len-- > 0)
    {
    int cc;
    int c = *p++;
    if (c == ' ') continue;
    if (c == '*') break;

    if ((ch_tab[c] & ch_hexch) != 0)
      {
      int uc = toupper(c);
      cc = (isalpha(uc)?  uc - 'A' + 10 : uc - '0') << 4;
      }
    else 
      {
      error_moan(58, c);
      ok = FALSE; 
      continue;  
      }

    c = *p++;
    len--;
    if ((ch_tab[c] & ch_hexch) != 0)
      {
      int uc = toupper(c);
      cc += isalpha(uc)?  uc - 'A' + 10 : uc - '0';
      }
    else { error_moan(58, c); ok = FALSE; }

    fputc(cc, f);
    }
    
  return ok? 1 : 0;
  }

/* Handle normal output; we need to scan the line only if it is
to have tabs inserted into it. First check for detrailing. */

if (main_detrail_output)
  {
  char *pp = p + len - 1;
  while (len > 0 && *pp-- == ' ') len--; 
  } 

if (main_tabout || (line->flags & lf_tabs) != 0)
  {
  for (i = 0; i < len; i++)
    {
    int c = p[i];

    /* Search for string of 2 or more spaces, ending at tabstop */

    if (c == ' ')
      {
      int n; 
      int k = i + 1;
      while (k < len) { if (p[k] != ' ') break; k++; }
      while (k > i + 1 && (k & 7) != 0) k--;

      if ((n = (k - i)) > 1 && (k & 7) == 0)
        {
        /* Found suitable string - use tab(s) and skip spaces */
        c = '\t';
        i = k - 1;
        for (; n > 8; n -= 8) fputc('\t', f);
        }
      }
    fputc(c, f);
    }
  }

/* Untabbed line -- optimize */

else fwrite(p, 1, len, f);

/* Add final LF */

fputc('\n', f);

/* Check that the line was successfully written, and yield result */

return ferror(f)? (-1) : (+1);
}



/*************************************************
*           Write current buffer to file         *
*************************************************/

/* This is used for entire buffers that are held in store */

BOOL file_save(char *name)
{
FILE *f;
linestr *line = main_top;
int yield = TRUE;

/* Ensure all in store before opening output, as it may be the
same file as the input. */

while (file_extend() != NULL);

if (name == NULL || name[0] == 0)
  { error_moan(59, currentbuffer->bufferno); return FALSE; }
else if (strcmp(name, "-") == 0) f = stdout;
else if ((f = sys_fopen(name, "w", NULL)) == NULL) 
  { error_moan(5, name); return FALSE; }

while ((line->flags & lf_eof) == 0)
  {
  int rc = file_writeline(line, f);
  if (rc < 0)
    {
    error_moan(37, name, strerror(errno));
    return FALSE;
    }
  else if (rc == 0) yield = FALSE;   /* Binary failure */
  line = line->next;
  }

if (f != stdout) 
  {
  fclose(f);
  sys_setfiletype(name, currentbuffer->filetype); 
  } 
   
return yield;
}



/*************************************************
*          Complete writing a stream buffer      *
*************************************************/

/* A stream buffer may already be partly written. In addition, we
do not want to attempt to load it all into store. The buffer will
be destroyed immediately after this function is obeyed. */

BOOL file_streamout(char *name)
{
FILE *f = currentbuffer->to_fid;
linestr *line = main_top;
int yield = TRUE;

/* First, write the lines that are already in store */

while (line != NULL && (line->flags & lf_eof) == 0)
  {
  int rc = file_writeline(line, f);
  if (rc < 0)
    {
    error_moan(37, name, strerror(errno));
    return FALSE;
    }
  else if (rc == 0) yield = FALSE; 
  line = line->next;
  }

/* If the input file is not closed, simply copy the rest over */

if (from_fid != NULL)
  {
  int count; 
  char buff[1024];
  while ((count = fread(buff, 1, 1024, from_fid)) > 0)
    {
    if (fwrite(buff, 1, count, f) != count)
      {
      error_moan(37, name, strerror(errno));
      return FALSE;
      }
    }
  fclose(from_fid);
  from_fid = NULL; 
  }

fclose(f);
currentbuffer->to_fid = NULL;
sys_setfiletype(currentbuffer->filename, currentbuffer->filetype);
return yield;
}

/* End of efile.c */
