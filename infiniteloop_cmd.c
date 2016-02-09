// Copyright (c) 2016 Ed Schouten <ed@nuxi.nl>
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "infiniteloop.h"

static unsigned int solutions_found = 0;

static bool print_solution(const struct il_solution *s, void *thunk) {
  char buf[IL_SOLUTION_PRINT_MAX];
  if (!il_solution_print(s, buf, sizeof(buf))) {
    fprintf(stderr, "Failed to print solution\n");
    exit(1);
  }
  printf("-- SOLUTION --\n%s\n", buf);
  ++solutions_found;
  return true;
}

int main(void) {
  char buf[1024];
  size_t len = fread(buf, 1, sizeof(buf) - 1, stdin);
  buf[len] = '\0';

  struct il_problem p;
  if (!il_problem_parse(buf, &p)) {
    fprintf(stderr, "Failed to parse input\n");
    return 1;
  }

  il_problem_solve(&p, print_solution, NULL);

  printf("-- FOUND %u SOLUTIONS --\n", solutions_found);
  return 0;
}
