// Copyright (c) 2016 Ed Schouten <ed@nuxi.nl>
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "infiniteloop.h"

#define TEST(a, b) int main(void)
#define ASSERT_TRUE(x) assert(x)

struct solve_param {
  const char *const *solutions;
  bool *found;
  size_t nsolutions;
};

static bool solve_callback(const struct il_solution *s, void *thunk) {
  char buf[IL_SOLUTION_PRINT_MAX];
  ASSERT_TRUE(il_solution_print(s, buf, sizeof(buf)));

  const struct solve_param *param = thunk;
  for (size_t i = 0; i < param->nsolutions; ++i) {
    if (!param->found[i] && strcmp(buf, param->solutions[i]) == 0) {
      param->found[i] = true;
      return true;
    }
  }
  ASSERT_TRUE(false);
}

static void do_test(const char *problem, const char *const *solutions,
                    size_t nsolutions) {
  // Parse test input string.
  struct il_problem p;
  ASSERT_TRUE(il_problem_parse(problem, &p));

  // Table of solutions already found.
  bool found[16];
  for (size_t i = 0; i < nsolutions; ++i)
    found[i] = false;

  struct solve_param param = {
      .solutions = solutions, .found = found, .nsolutions = nsolutions,
  };
  il_problem_solve(&p, solve_callback, &param);

  for (size_t i = 0; i < nsolutions; ++i)
    ASSERT_TRUE(found[i]);
}

#define EXAMPLE(problem, ...)                                              \
  do {                                                                     \
    const char *const solutions[] = {__VA_ARGS__};                         \
    do_test(problem, solutions, sizeof(solutions) / sizeof(solutions[0])); \
  } while (0)

struct resolve_param {
  char expected[IL_SOLUTION_PRINT_MAX];
  bool found;
};

static bool resolve_callback(const struct il_solution *s, void *thunk) {
  struct resolve_param *param = thunk;
  char buf[sizeof(param->expected)];
  ASSERT_TRUE(il_solution_print(s, buf, sizeof(buf)));
  if (strcmp(buf, param->expected) == 0)
    param->found = true;
  return !param->found;
}

TEST(il_solve, examples) {
  // Empty puzzles.
  EXAMPLE("", "");
  EXAMPLE("    \n\n      ", "");

  // Puzzle that cannot be solved.
  EXAMPLE("1sssss");

  // Puzzle with multiple answers.
  EXAMPLE(
      "1cc1\n"
      "1cc1",
      "╶──╮  ╭──╴\n"
      "   │  │\n"
      "╶──╯  ╰──╴",
      "╷  ╭──╮  ╷\n"
      "│  │  │  │\n"
      "╵  ╰──╯  ╵");

  EXAMPLE(
      "11  11\n"
      "CC11CC\n"
      "C4SS4C\n"
      " 1  1\n"
      "C3333C\n"
      "11CC11",
      "╶──╴        ╶──╴\n"
      "\n"
      "╭──╮  ╶──╴  ╭──╮\n"
      "│  │        │  │\n"
      "╰──┼────────┼──╯\n"
      "   │        │\n"
      "   ╵        ╵\n"
      "\n"
      "╭──┬──┬──┬──┬──╮\n"
      "│  │  │  │  │  │\n"
      "╵  ╵  ╰──╯  ╵  ╵");

  // Puzzle #166.
  EXAMPLE(
      "1C1C11\n"
      " CCC11\n"
      "CC  C1\n"
      "S331S1\n"
      "CCSCCS\n"
      "C11S1S\n"
      "S 133S\n"
      "S SSC3\n"
      "3C331S\n"
      "CC11CS\n"
      " CC143\n"
      " CC1C1\n",
      "╶──╮  ╷  ╭──╴  ╷\n"
      "   │  │  │     │\n"
      "   ╰──╯  ╰──╴  ╵\n"
      "\n"
      "╭──╮        ╭──╴\n"
      "│  │        │\n"
      "│  ├──┬──╴  │  ╷\n"
      "│  │  │     │  │\n"
      "╰──╯  │  ╭──╯  │\n"
      "      │  │     │\n"
      "╭──╴  ╵  │  ╷  │\n"
      "│        │  │  │\n"
      "│     ╷  ├──┤  │\n"
      "│     │  │  │  │\n"
      "│     │  │  ╰──┤\n"
      "│     │  │     │\n"
      "├──╮  ├──┴──╴  │\n"
      "│  │  │        │\n"
      "╰──╯  ╵  ╶──╮  │\n"
      "            │  │\n"
      "   ╭──╮  ╶──┼──┤\n"
      "   │  │     │  │\n"
      "   ╰──╯  ╶──╯  ╵");

  for (int i = 0; i < 100; ++i) {
    // Generate a random solution and store the expected textual
    // representation.
    struct il_solution s;
    for (size_t x = 0; x < IL_AXIS - 3; ++x)
      for (size_t y = 0; y < IL_AXIS - 2; ++y)
        s.horizontal[x][y] = arc4random_uniform(2);
    for (size_t x = 0; x < IL_AXIS - 2; ++x)
      for (size_t y = 0; y < IL_AXIS - 3; ++y)
        s.vertical[x][y] = arc4random_uniform(2);
    struct resolve_param param;
    ASSERT_TRUE(il_solution_print(&s, param.expected, sizeof(param.expected)));

    // Convert the solution back to a problem and try to solve it. We
    // should see the original solution appear once.
    struct il_problem p;
    il_solution_unsolve(&s, &p);
    param.found = false;
    il_problem_solve(&p, resolve_callback, &param);
    ASSERT_TRUE(param.found);
  }
}
