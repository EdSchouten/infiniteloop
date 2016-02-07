#ifndef INFINITELOOP_H
#define INFINITELOOP_H

#include <stdbool.h>
#include <stddef.h>

// Maximum board axis width.
#define IL_AXIS 16

// Puzzle input structure.
//
// This structure stores the expected piece for every cell on the board.
// Pieces are encoded as a bitmask, where the lower four bits determine
// the shape of the piece. For example, a value of zero denotes an empty
// cell, whereas a value of 0xf denotes a cross. A corner piece could be
// encoded by using the value 0x3, for example.
//
// The outer border of the board must never be used, as it is merely
// used by the algorithm to prevent the need for bounds checking.
struct il_problem {
  unsigned char board[IL_AXIS][IL_AXIS];
};

// Puzzle output structure.
//
// This structure stores a solution for a puzzle, storing which
// horizontal and vertical edges are set.
struct il_solution {
  bool horizontal[IL_AXIS - 3][IL_AXIS - 2];
  bool vertical[IL_AXIS - 2][IL_AXIS - 3];
};

// Parses a string encoding the layout of a puzzle input.
bool il_problem_parse(const char *, struct il_problem *);

// Generates a string encoding the layout of a puzzle output.
bool il_solution_print(const struct il_solution *, char *, size_t);

// Generates all solutions for a puzzle. The callback is invoked for
// every solution. Additional solutions are computed if the callback
// returns true.
void il_solve(const struct il_problem *,
              bool (*)(const struct il_solution *, void *), void *);

#endif
