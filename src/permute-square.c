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
#include <stdio.h>
#include <gmp.h>
#include <argp.h>
#include <string.h>
#include <stdlib.h>
#include "magicsquareutil.h"
//http://stackoverflow.com/questions/2710713/algorithm-to-generate-all-possible-permutations-of-a-list by user Horacio in 2015.

int (*read_numbers)(FILE *, mpz_t *, int, char **, size_t *) = read_numbers_from_stream;
void (*display_record) (mpz_t *, FILE *) = display_nine_record;

int invert;
int show_all;
int display_offset;
int fact;

void Rotate(mpz_t vec[], int size)
{
  int i, j;
  mpz_t first;
  mpz_init (first);

  mpz_set (first, vec[0]);
  for(j = 0, i = 1; i < size; i++, j++)
    mpz_set (vec[j], vec[i]);
  mpz_set (vec[j], first);
  mpz_clear (first);
}

int Permutate(mpz_t *start, int size, int *count)
{
  if(size > 1)
    {
      if(Permutate(start + 1, size - 1, count))
        {
          Rotate(start, size);
        }
      fact *= size;
    }
  else
    {
      (*count)++;
      fact = 1;
    }

  return !(*count % fact);
}

static void
show_offset (mpz_t orig[], mpz_t vec[], int size, FILE *out)
{
  for (int i = 0; i < size; i++)
    {
      for (int j = 0; j < size; j++)
        {
          if (mpz_cmp (orig[i], vec[j]) == 0)
            {
              fprintf (out, "%d, ", j - i);
            }
        }
    }
  fprintf (out, "\n");
}

static int
should_stop (mpz_t vec[], int size)
{
  int count = 0;
  mpz_t a[3][3];
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      {
        mpz_init (a[i][j]);
        mpz_set (a[i][j], vec[count]);
        count++;
      }
  int ret = is_magic_square (a, 0);
  for (int i = 0; i < 3; i++)
    for (int j = 0; j < 3; j++)
      mpz_clear (a[i][j]);
  return ret;
}

static int
find_combinations (FILE *stream)
{
  char *line = NULL;
  size_t len = 0;
  ssize_t read;

  mpz_t vec[SIZE];
  mpz_t orig[SIZE];
  for (int i = 0; i < SIZE; i++)
    mpz_inits (vec[i], orig[i], NULL);
  int count = 0;
  while (1)
    {
      int found = 0;
      read = read_numbers (stream, vec, SIZE, &line, &len);
      if (read == -1)
        break;
      if (display_offset)
        {
          for (int i = 0; i < SIZE; i++)
            mpz_set (orig[i], vec[i]);
        }
      fact = 0;
      count = 0;
      do
        {
          if (show_all)
            {
              if (display_offset)
                show_offset (orig, vec, SIZE, stdout);
              else
                display_record (vec, stdout);
            }
          else
            {
              if (should_stop (vec, SIZE))
                {
                  found = 1;
                  if (!invert)
                    {
                      if (display_offset)
                        show_offset (orig, vec, SIZE, stdout);
                      else
                        display_record (vec, stdout);
                    }
                  break;
                }
            }
        } while(!Permutate(vec, SIZE, &count));
      if (!found && !show_all && invert)
        display_record (vec, stdout);
    }

  for (int i = 0; i < SIZE; i++)
    mpz_clears (vec[i], orig[i], NULL);
  if (line)
    free (line);
  return 0;
}

static error_t
parse_opt (int key, char *arg, struct argp_state *state)
{
  switch (key)
    {
    case 'v':
      invert = 1;
      break;
    case 'o':
      display_record = display_binary_nine_record;
      break;
    case 'i':
      read_numbers = binary_read_numbers_from_stream;
      break;
    case 'S':
      show_all = 1;
      break;
    case 'd':
      display_offset = 1;
      break;
    }
  return 0;
}

static struct argp_option
options[] =
{
  { "in-binary", 'i', 0, 0, "Input raw GMP numbers instead of text"},
  { "out-binary", 'o', 0, 0, "Output raw GMP numbers instead of text"},
  { "show-all", 'S', 0, 0, "Just permute, don't look for a magic square"},
  { "display-offset", 'd', 0, 0, "Display the relative position changes"},
  { "invert", 'v', 0, 0, "Show the squares that we can't permute into magic squares"},
  { 0 }
};

struct argp argp ={options, parse_opt, 0, "Accept 9 numbers from the standard input, and permute them to find a magic sqaure.\vThe nine values must be separated by a comma and terminated by a newline." , 0};

int
main (int argc, char **argv)
{
  setenv ("ARGP_HELP_FMT", "no-dup-args-note", 1);
  argp_parse (&argp, argc, argv, 0, 0, 0);
  is_magic_square_init ();
  return find_combinations (stdin);
}
