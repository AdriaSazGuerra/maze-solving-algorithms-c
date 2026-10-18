# Maze-Solving Algorithms in C

A command-line maze solver implemented in C and developed as a computational mathematics / programming project.

## Implemented methods

1. **Random walk** — stochastic baseline
2. **Recursive backtracking** — depth-first recursive exploration
3. **Breadth-first search (BFS)** — queue-based graph traversal with shortest-path reconstruction

## Technical features

- dynamic memory allocation for maze input and solver state
- linked-list queue implementation for BFS
- visited-state tracking and path reconstruction
- command-line input validation and optional diagnostic output
- explicit handling of memory-allocation and malformed-input errors

## Build

```bash
gcc -std=c99 -O2 -Wall -Wextra maze_solver.c -o maze_solver
```

The original coursework accepts a text maze file and a solver mode as command-line arguments.

## Author

**Adrià Saz Guerra**
