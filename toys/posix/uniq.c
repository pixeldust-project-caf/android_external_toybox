/* uniq.c - report or filter out repeated lines in a file
 *
 * Copyright 2012 Georgi Chorbadzhiyski <georgi@unixsol.org>
 *
 * See http://opengroup.org/onlinepubs/9699919799/utilities/uniq.html

USE_UNIQ(NEWTOY(uniq, "f#s#w#zicdu", TOYFLAG_USR|TOYFLAG_BIN))

config UNIQ
  bool "uniq"
  default y
  help
    usage: uniq [-cduiz] [-w maxchars] [-f fields] [-s char] [input_file [output_file]]

    Report or filter out repeated lines in a file

    -c	show counts before each line
    -d	show only lines that are repeated
    -u	show only lines that are unique
    -i	ignore case when comparing lines
    -z	lines end with \0 not \n
    -w	compare maximum X chars per line
    -f	ignore first X fields
    -s	ignore first X chars
*/

#define FOR_uniq
#include "toys.h"

GLOBALS(
  long w, s, f;

  long repeats;
)

static char *skip(char *str)
{
  long nchars = TT.s, nfields = TT.f;

  // Skip fields first
  while (nfields--) {
    while (*str && isspace(*str)) str++;
    while (*str && !isspace(*str)) str++;
  }
  // Skip chars
  while (*str && nchars--) str++;

  return str;
}

static void print_line(FILE *f, char *line)
{
  if (toys.optflags & (TT.repeats ? FLAG_u : FLAG_d)) return;
  if (toys.optflags & FLAG_c) fprintf(f, "%7lu ", TT.repeats + 1);
  fputs(line, f);
  if (toys.optflags & FLAG_z) fputc(0, f);
}

void uniq_main(void)
{
  FILE *infile = stdin, *outfile = stdout;
  char *thisline = 0, *prevline = 0, *tmpline, eol = '\n';
  size_t thissize, prevsize = 0, tmpsize;

  if (toys.optc >= 1) infile = xfopen(toys.optargs[0], "r");
  if (toys.optc >= 2) outfile = xfopen(toys.optargs[1], "w");

  if (toys.optflags & FLAG_z) eol = 0;

  // If first line can't be read
  if (getdelim(&prevline, &prevsize, eol, infile) < 0) return;

  while (getdelim(&thisline, &thissize, eol, infile) > 0) {
    int diff;
    char *t1, *t2;

    // If requested get the chosen fields + character offsets.
    if (TT.f || TT.s) {
      t1 = skip(thisline);
      t2 = skip(prevline);
    } else {
      t1 = thisline;
      t2 = prevline;
    }

    if (!TT.w)
      diff = !(toys.optflags & FLAG_i) ? strcmp(t1, t2) : strcasecmp(t1, t2);
    else diff = !(toys.optflags & FLAG_i) ? strncmp(t1, t2, TT.w)
                  : strncasecmp(t1, t2, TT.w);

    if (!diff) TT.repeats++;
    else {
      print_line(outfile, prevline);

      TT.repeats = 0;

      tmpline = prevline;
      prevline = thisline;
      thisline = tmpline;

      tmpsize = prevsize;
      prevsize = thissize;
      thissize = tmpsize;
    }
  }

  print_line(outfile, prevline);

  if (CFG_TOYBOX_FREE) {
    if (outfile != stdout) fclose(outfile);
    if (infile != stdin) fclose(infile);
    free(prevline);
    free(thisline);
  }
}
