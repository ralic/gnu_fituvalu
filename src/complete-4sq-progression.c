/* Copyright (C) 2016, 2017 Ben Asselstine
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <argp.h>
#include <gmp.h>
#include "magicsquareutil.h"

int in_binary;
nine_progression_t *nine_prog = &nine_progressions[0];
four_square_progression_t *four_square_prog;
void (*display_record) (mpz_t *progression, FILE *out) = display_nine_record;

void (*four_to_nine_prog)(mpz_t *, mpz_t, mpz_t, mpz_t, mpz_t);
int (*count_prog) (mpz_t *);
int filter;

static void
count_squares_and_display_progression (mpz_t *progression, FILE *out)
{
  int count = count_prog (progression);
  if (count >= filter)
    display_record (progression, out);
}

static void
display_progression (mpz_t *progression, FILE *out)
{
  display_record (progression, out);
}

static void
complete_four_square_progression (FILE *in, FILE *out)
{
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  mpz_t i[4];
  mpz_t p[9];
  mpz_inits (i[0], i[1], i[2], i[3], NULL);
  mpz_inits (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], NULL);

  static void (*func) (mpz_t *, FILE *);

  if (filter)
    func = count_squares_and_display_progression;
  else
    func = display_progression;

  while (1)
    {
      for (int j = 0; j < 4; j++)
        {
          if (j == 3)
            read = getline (&line, &len, in);
          else
            read = getdelim (&line, &len, ',', in);
          if (read == -1)
            break;
          char *end = strpbrk (line, ",\n");
          if (end)
            *end = '\0';
          mpz_set_str (i[j], line, 10);
        }
      if (read == -1)
        break;
      four_to_nine_prog (p, i[0], i[1], i[2], i[3]);
      func (p, out);
    }
  mpz_clears (i[0], i[1], i[2], i[3], NULL);
  mpz_clears (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], NULL);
  free (line);
}

static void
complete_four_square_progression_binary (FILE *in, FILE *out)
{
  ssize_t read;
  mpz_t i[4];
  mpz_t p[9];
  mpz_inits (i[0], i[1], i[2], i[3], NULL);
  mpz_inits (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], NULL);

  static void (*func) (mpz_t *, FILE *);

  if (filter)
    func = count_squares_and_display_progression;
  else
    func = display_progression;

  while (1)
    {
      for (int j = 0; j < 4; j++)
        {
          read = mpz_inp_raw (i[j], in);
          if (!read)
            break;
        }
      if (!read)
        break;
      four_to_nine_prog (p, i[0], i[1], i[2], i[3]);
      func (p, out);
    }
  mpz_clears (i[0], i[1], i[2], i[3], NULL);
  mpz_clears (p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7], p[8], NULL);
}

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'o':
      display_record = display_binary_nine_record;
      break;
    case 'i':
      in_binary = 1;
      break;
    case 'f':
      filter = atoi (arg);
      break;
    case 't':
        {
          nine_progression_t *p =
            lookup_nine_progression_by_name (arg);
          if (p)
            nine_prog = p;
          else
            {
              char *types = generate_list_of_nine_progression_types ();
              argp_error (state, "--type must be one of %s.", types);
              free (types);
            }
        }
      break;
    case ARGP_KEY_FINI:
      four_to_nine_prog = nine_prog->progfunc;
      count_prog = nine_prog->countfunc;
      four_square_prog =
        lookup_four_square_progression_by_kind
        (nine_prog->four_square_progression);
      break;
    }
  return 0;
}

static char *
help_filter (int key, const char *text, void *input)
{
  if (key == ARGP_KEY_HELP_POST_DOC)
    {
      char *s = generate_list_of_nine_progression_timelines ();
      char *q = NULL;
      asprintf (&q, "\n%s", s);
      free (s);
      char *new_text = NULL;
      if (asprintf (&new_text, text, q) != -1)
        return new_text;
    }
  return (char *) text;
}

static struct argp_option
options[] =
{
  { "in-binary", 'i', 0, 0, "Input raw GMP numbers instead of text"},
  { "out-binary", 'o', 0, 0, "Output raw GMP numbers instead of text"},
  { "type", 't', "NAME", 0, "Use NAME as the 9 number progression strategy"},
  { "filter", 'f', "NUM", 0, "Only show progressions that have at least NUM perfect squares" },
  { 0 }
};

struct argp argp ={options, parse_opt, NULL, "Read 4 perfect squares from the standard input and complete the progression to 9 numbers.\vThe four values must be separated by a comma and termined by a newline.  --type must be one of:%s", 0, help_filter};

int
main (int argc, char **argv)
{
  setenv ("ARGP_HELP_FMT", "no-dup-args-note", 1);
  argp_parse (&argp, argc, argv, 0, 0, 0);
  if (in_binary)
    complete_four_square_progression_binary (stdin, stdout);
  else
    complete_four_square_progression (stdin, stdout);
  return 0;
}
