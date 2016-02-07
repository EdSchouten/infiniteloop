// Copyright (c) 2016 Ed Schouten <ed@nuxi.nl>
//
// This file is distributed under a 2-clause BSD license.
// See the LICENSE file for details.

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "infiniteloop.h"

static bool dpll(const struct il_problem *, unsigned char[IL_AXIS][IL_AXIS],
                 bool (*)(const struct il_solution *, void *), void *);

bool il_problem_parse(const char *in, struct il_problem *p) {
  // Throw away the existing board.
  for (size_t x = 0; x < IL_AXIS; ++x)
    for (size_t y = 0; y < IL_AXIS; ++y)
      p->board[x][y] = 0;

  // Parse the input string.
  size_t x = 1, y = 1;
  for (;;) {
    switch (*in++) {
      case '\0':
        return true;

      // Cursor movement characters.
      case ' ':
        ++x;
        break;
      case '\n':
        x = 1;
        ++y;
        break;

      // Cell shapes.
      case '1':
        // A dead end.
        if (x >= IL_AXIS - 1 || y >= IL_AXIS - 1)
          return false;
        p->board[x++][y] = 0x1;
        break;
      case 'C':
        // A corner.
        if (x >= IL_AXIS - 1 || y >= IL_AXIS - 1)
          return false;
        p->board[x++][y] = 0x3;
        break;
      case 'S':
        // A straight line.
        if (x >= IL_AXIS - 1 || y >= IL_AXIS - 1)
          return false;
        p->board[x++][y] = 0x5;
        break;
      case '3':
        // A three-way junction.
        if (x >= IL_AXIS - 1 || y >= IL_AXIS - 1)
          return false;
        p->board[x++][y] = 0x7;
        break;
      case '4':
        // A crossing.
        if (x >= IL_AXIS - 1 || y >= IL_AXIS - 1)
          return false;
        p->board[x++][y] = 0xf;
        break;
    }
  }
}

// Rotates a cell clockwise by i steps. The number of steps has to be
// provided in the form 1 << i.
static unsigned char rotate(unsigned char a, unsigned char b) {
  unsigned char v = a * b;
  return (v | (v >> 4)) & 0xf;
}

// Rotates a cell by 2 steps, effectively turning the cell upside down.
static unsigned char rotate2(unsigned char c) {
  return ((c << 2) | (c >> 2)) & 0xf;
}

// Determines the union of all of the edges that could be set if the
// cell is rotated by any number of steps encoded in a bitmask. This
// function is effectively identical to:
//
// fanout(a, b) == rotate(a, b & 0x1) | rotate(a, b & 0x2) |
//                 rotate(a, b & 0x4) | rotate(a, b & 0x8)
static unsigned char fanout(unsigned char a, unsigned char b) {
  unsigned char v =
      a * (b & 0x1) | a * (b & 0x2) | a * (b & 0x4) | a * (b & 0x8);
  return (v | (v >> 4)) & 0xf;
}

// Returns true if the cell only has a single edge set.
static bool single_bit_set(unsigned char c) {
  return (c & (c - 1)) == 0;
}

// Returns true if a solution has been fully computed. This means that
// every cell can only be placed in exactly one way.
static bool finished(const unsigned char options[IL_AXIS][IL_AXIS]) {
  for (size_t x = 1; x < IL_AXIS - 1; ++x)
    for (size_t y = 1; y < IL_AXIS - 1; ++y)
      if (!single_bit_set(options[x][y]))
        return false;
  return true;
}

// Performs the propagation step as performed by the DPLL algorithm.
//
// This function takes an partial solution to a problem and reduces
// possible placement of cells by inference. By looking at neighbouring
// cells, it determines in which way a cell could be placed. When
// discovering a contradiction, this function returns false.
//
// Execution of this function terminates if no more inference steps can
// be taken.
static bool propagate(const struct il_problem *p,
                      unsigned char options[IL_AXIS][IL_AXIS]) {
  bool made_change;
  do {
    made_change = false;
    for (size_t x = 1; x < IL_AXIS - 1; ++x)
      for (size_t y = 1; y < IL_AXIS - 1; ++y) {
#define YES(x, y, idx) (fanout(p->board[x][y], options[x][y]) & (1 << idx))
        // Determine which edges may be present.
        unsigned char may_be_set = rotate2(YES(x, y + 1, 0) | YES(x - 1, y, 1) |
                                           YES(x, y - 1, 2) | YES(x + 1, y, 3));
#undef YES
#define NO(x, y, idx) (fanout(p->board[x][y] ^ 0xf, options[x][y]) & (1 << idx))
        // Determine which edges may be absent.
        unsigned char may_be_clear = rotate2(NO(x, y + 1, 0) | NO(x - 1, y, 1) |
                                             NO(x, y - 1, 2) | NO(x + 1, y, 3));
#undef NO

        // Compute ways in which this cell may be placed by using the
        // bitmasks obtained above.
        unsigned char new_options = 0;
        for (unsigned char i = 0x1; i <= 0x8; i <<= 1) {
          if ((options[x][y] & i) != 0) {
            unsigned char c = rotate(p->board[x][y], i);
            if ((c & ~may_be_set) == 0 && (c | may_be_clear) == 0xf)
              new_options |= i;
          }
        }

        if (new_options != options[x][y]) {
          // Fail if the cell cannot be placed in any direction.
          if (new_options == 0)
            return false;
          made_change = true;
        }
        options[x][y] = new_options;
      }
  } while (made_change);
  return true;
}

// Reports a valid solution to the caller.
static bool report(const struct il_problem *p,
                   const unsigned char options[IL_AXIS][IL_AXIS],
                   bool (*callback)(const struct il_solution *, void *),
                   void *thunk) {
  // Extract edges from board.
  struct il_solution s;
  for (size_t x = 0; x < IL_AXIS - 3; ++x)
    for (size_t y = 0; y < IL_AXIS - 2; ++y)
      s.horizontal[x][y] =
          rotate(p->board[x + 1][y + 1], options[x + 1][y + 1]) & 0x2;
  for (size_t x = 0; x < IL_AXIS - 2; ++x)
    for (size_t y = 0; y < IL_AXIS - 3; ++y)
      s.vertical[x][y] =
          rotate(p->board[x + 1][y + 1], options[x + 1][y + 1]) & 0x4;

  // Invoke the user-supplied callback.
  return callback(&s, thunk);
}

// Performs the recursion step as part of the DPLL algorithm.
//
// If inference is unable to obtain a full solution (e.g., due to
// ambiguities), this function can be used to traverse the solution
// space. It selects a random cell that still has multiple solutions and
// reinvokes the DPLL algorithm by placing that cell in all allowed
// directions.
static bool guess(const struct il_problem *p,
                  const unsigned char options[IL_AXIS][IL_AXIS],
                  bool (*callback)(const struct il_solution *, void *),
                  void *thunk) {
  // Pick a random cell with multiple solutions.
  size_t x, y;
  do {
    size_t u = arc4random_uniform(IL_AXIS * IL_AXIS);
    x = u / IL_AXIS;
    y = u % IL_AXIS;
  } while (single_bit_set(options[x][y]));

  // Reinvoke the DPLL algorithm with all allowed directions.
  for (unsigned char i = 0x1; i <= 0x8; i <<= 1) {
    if ((options[x][y] & i) != 0) {
      unsigned char new_options[IL_AXIS][IL_AXIS];
      memcpy(new_options, options, sizeof(new_options));
      new_options[x][y] = i;
      if (!dpll(p, new_options, callback, thunk))
        return false;
    }
  }
  return true;
}

// Perform the DPLL algorithm.
//
// The DPLL algorithm starts out by inferring as many cell positions as
// possible. If this already yields a valid solution, we report it back
// to the caller. If not, we perform backtracking and run the algorithm
// once more.
static bool dpll(const struct il_problem *p,
                 unsigned char options[IL_AXIS][IL_AXIS],
                 bool (*callback)(const struct il_solution *, void *),
                 void *thunk) {
  return !propagate(p, options) ||
         (finished(options) ? report : guess)(p, options, callback, thunk);
}

void il_solve(const struct il_problem *p,
              bool (*callback)(const struct il_solution *, void *),
              void *thunk) {
  // Table of valid options remaining for every cell. Initialize it by
  // allowing all cells to be rotated to all four directions, except for
  // shapes that have rotational symmetry. For these shapes, we only
  // need them to be tried in one or two directions.
  unsigned char options[IL_AXIS][IL_AXIS];
  for (size_t x = 0; x < IL_AXIS; ++x)
    for (size_t y = 0; y < IL_AXIS; ++y)
      options[x][y] =
          (p->board[x][y] == 0 || p->board[x][y] == 0xf)
              ? 0x1
              : p->board[x][y] >> 2 == (p->board[x][y] & 0x3) ? 0x3 : 0x0f;

  // Invoke the DPLL algorithm to compute solutions.
  dpll(p, options, callback, thunk);
}

// Appends a string to the output buffer.
static bool putstr(char **out, size_t *outlen, const char *in) {
  size_t inlen = strlen(in);
  if (inlen >= *outlen)
    return false;
  memcpy(*out, in, inlen + 1);
  *out += inlen;
  *outlen -= inlen;
  return true;
}

// Writes whitespace to the output buffer until a given position has
// been reached.
static bool whitespace(char **out, size_t *outlen, size_t x, size_t y,
                       size_t *posx, size_t *posy) {
  while (y > *posy) {
    if (!putstr(out, outlen, "\n"))
      return false;
    *posx = 0;
    ++*posy;
  }
  while (x > *posx) {
    if (!putstr(out, outlen, " "))
      return false;
    ++*posx;
  }
  return true;
}

bool il_solution_print(const struct il_solution *s, char *out, size_t outlen) {
  // Empty output buffer.
  if (outlen == 0)
    return false;
  *out = '\0';

  size_t posx = 0, posy = 0;
  for (size_t y = 0; y < IL_AXIS - 2; ++y) {
    // Print lines containing cells.
    for (size_t x = 0; x < IL_AXIS - 2; ++x) {
      // Determine which outgoing edges a cell has.
      unsigned char idx = (y > 0 && s->vertical[x][y - 1] ? 0x1 : 0) |
                          (x < IL_AXIS - 3 && s->horizontal[x][y] ? 0x2 : 0) |
                          (y < IL_AXIS - 3 && s->vertical[x][y] ? 0x4 : 0) |
                          (x > 0 && s->horizontal[x - 1][y] ? 0x8 : 0);
      if (idx != 0) {
        // Print cell.
        const char cells[16][4] = {"",  "╵", "╶", "╰", "╷", "│", "╭", "├",
                                   "╴", "╯", "─", "┴", "╮", "┤", "┬", "┼"};
        if (!whitespace(&out, &outlen, 2 * x, 2 * y, &posx, &posy))
          return false;
        if (!putstr(&out, &outlen, cells[idx]))
          return false;
        ++posx;

        // Print horizontal edges.
        if (x < IL_AXIS - 3 && s->horizontal[x][y]) {
          if (!putstr(&out, &outlen, "─"))
            return false;
          ++posx;
        }
      }
    }

    // Print vertical edges.
    if (y < IL_AXIS - 3) {
      for (size_t x = 0; x < IL_AXIS - 2; ++x) {
        if (s->vertical[x][y]) {
          if (!whitespace(&out, &outlen, 2 * x, 2 * y + 1, &posx, &posy))
            return false;
          if (!putstr(&out, &outlen, "│"))
            return false;
          ++posx;
        }
      }
    }
  }
  return true;
}
