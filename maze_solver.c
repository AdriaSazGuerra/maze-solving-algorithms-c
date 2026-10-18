/*
 * Maze-solving algorithms in C
 * Adrià Saz Guerra
 *
 * Public portfolio refactor of a university programming project.
 * Implements three solution strategies for ASCII mazes:
 *   1) random walk
 *   2) recursive backtracking
 *   3) breadth-first search (BFS)
 *
 * Maze convention used here:
 *   '#' = wall
 *   'S' = start
 *   'E' = exit
 *   ' ' = free cell
 *
 * Build: gcc -std=c99 -O2 -Wall -Wextra maze_solver.c -o maze_solver
 * Usage: ./maze_solver <random|backtrack|bfs> <maze.txt>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
    int row;
    int col;
} Position;

typedef struct Node {
    Position value;
    struct Node *next;
} Node;

typedef struct {
    Node *front;
    Node *back;
} Queue;

typedef struct {
    char **cells;
    int rows;
    int cols;
    Position start;
    Position exit;
} Maze;

static const int DR[4] = {-1, 1, 0, 0};
static const int DC[4] = {0, 0, -1, 1};
static const char *DIR_NAME[4] = {"north", "south", "west", "east"};

static int same_position(Position a, Position b) {
    return a.row == b.row && a.col == b.col;
}

static int in_bounds(const Maze *maze, int row, int col) {
    return row >= 0 && row < maze->rows && col >= 0 && col < maze->cols;
}

static int is_open(const Maze *maze, int row, int col) {
    return in_bounds(maze, row, col) && maze->cells[row][col] != '#';
}

static void free_maze(Maze *maze) {
    if (!maze || !maze->cells) return;
    for (int r = 0; r < maze->rows; ++r) free(maze->cells[r]);
    free(maze->cells);
    maze->cells = NULL;
}

static int load_maze(const char *path, Maze *maze) {
    FILE *fp = fopen(path, "r");
    if (!fp) {
        fprintf(stderr, "Could not open '%s'.\n", path);
        return 0;
    }

    char buffer[4096];
    int rows = 0;
    int cols = -1;
    char **cells = NULL;
    Position start = {-1, -1};
    Position exit = {-1, -1};

    while (fgets(buffer, sizeof(buffer), fp)) {
        size_t len = strcspn(buffer, "\r\n");
        buffer[len] = '\0';
        if (cols < 0) cols = (int)len;
        if ((int)len != cols) {
            fprintf(stderr, "Maze must be rectangular.\n");
            fclose(fp);
            for (int r = 0; r < rows; ++r) free(cells[r]);
            free(cells);
            return 0;
        }

        char **tmp = realloc(cells, (rows + 1) * sizeof(*cells));
        if (!tmp) {
            fprintf(stderr, "Memory allocation failed.\n");
            fclose(fp);
            for (int r = 0; r < rows; ++r) free(cells[r]);
            free(cells);
            return 0;
        }
        cells = tmp;
        cells[rows] = malloc((size_t)cols + 1);
        if (!cells[rows]) {
            fprintf(stderr, "Memory allocation failed.\n");
            fclose(fp);
            for (int r = 0; r < rows; ++r) free(cells[r]);
            free(cells);
            return 0;
        }
        strcpy(cells[rows], buffer);

        for (int c = 0; c < cols; ++c) {
            if (buffer[c] == 'S') start = (Position){rows, c};
            if (buffer[c] == 'E') exit = (Position){rows, c};
        }
        ++rows;
    }
    fclose(fp);

    if (rows == 0 || cols <= 0 || start.row < 0 || exit.row < 0) {
        fprintf(stderr, "Maze must contain both 'S' and 'E'.\n");
        for (int r = 0; r < rows; ++r) free(cells[r]);
        free(cells);
        return 0;
    }

    maze->cells = cells;
    maze->rows = rows;
    maze->cols = cols;
    maze->start = start;
    maze->exit = exit;
    return 1;
}

static Queue *queue_create(void) {
    Queue *q = malloc(sizeof(*q));
    if (!q) return NULL;
    q->front = q->back = NULL;
    return q;
}

static int queue_empty(const Queue *q) {
    return q->front == NULL;
}

static int queue_push(Queue *q, Position p) {
    Node *node = malloc(sizeof(*node));
    if (!node) return 0;
    node->value = p;
    node->next = NULL;
    if (q->back) q->back->next = node;
    else q->front = node;
    q->back = node;
    return 1;
}

static Position queue_pop(Queue *q) {
    Node *node = q->front;
    Position p = node->value;
    q->front = node->next;
    if (!q->front) q->back = NULL;
    free(node);
    return p;
}

static void queue_destroy(Queue *q) {
    while (q && !queue_empty(q)) (void)queue_pop(q);
    free(q);
}

static void print_path(Position *path, int length) {
    printf("Path length: %d steps\n", length - 1);
    for (int i = 1; i < length; ++i) {
        int dr = path[i].row - path[i - 1].row;
        int dc = path[i].col - path[i - 1].col;
        if (dr == -1) printf("north ");
        else if (dr == 1) printf("south ");
        else if (dc == -1) printf("west ");
        else if (dc == 1) printf("east ");
    }
    putchar('\n');
}

static int solve_bfs(const Maze *maze) {
    const int n = maze->rows * maze->cols;
    unsigned char *visited = calloc((size_t)n, 1);
    Position *parent = malloc((size_t)n * sizeof(*parent));
    if (!visited || !parent) {
        free(visited); free(parent);
        return 0;
    }

    for (int i = 0; i < n; ++i) parent[i] = (Position){-1, -1};

    Queue *q = queue_create();
    if (!q || !queue_push(q, maze->start)) {
        queue_destroy(q); free(visited); free(parent);
        return 0;
    }
    visited[maze->start.row * maze->cols + maze->start.col] = 1;

    int found = 0;
    while (!queue_empty(q)) {
        Position cur = queue_pop(q);
        if (same_position(cur, maze->exit)) {
            found = 1;
            break;
        }

        for (int d = 0; d < 4; ++d) {
            Position nxt = {cur.row + DR[d], cur.col + DC[d]};
            if (!is_open(maze, nxt.row, nxt.col)) continue;
            int idx = nxt.row * maze->cols + nxt.col;
            if (visited[idx]) continue;
            visited[idx] = 1;
            parent[idx] = cur;
            if (!queue_push(q, nxt)) {
                queue_destroy(q); free(visited); free(parent);
                return 0;
            }
        }
    }
    queue_destroy(q);

    if (!found) {
        printf("No path found.\n");
        free(visited); free(parent);
        return 0;
    }

    Position *reverse = malloc((size_t)n * sizeof(*reverse));
    if (!reverse) {
        free(visited); free(parent);
        return 0;
    }

    int length = 0;
    for (Position p = maze->exit; p.row >= 0; p = parent[p.row * maze->cols + p.col]) {
        reverse[length++] = p;
        if (same_position(p, maze->start)) break;
    }

    Position *path = malloc((size_t)length * sizeof(*path));
    if (!path) {
        free(reverse); free(visited); free(parent);
        return 0;
    }
    for (int i = 0; i < length; ++i) path[i] = reverse[length - 1 - i];
    print_path(path, length);

    free(path); free(reverse); free(visited); free(parent);
    return 1;
}

static int backtrack_dfs(const Maze *maze, Position cur, unsigned char *visited,
                         Position *path, int depth, int *path_length) {
    int idx = cur.row * maze->cols + cur.col;
    visited[idx] = 1;
    path[depth] = cur;

    if (same_position(cur, maze->exit)) {
        *path_length = depth + 1;
        return 1;
    }

    for (int d = 0; d < 4; ++d) {
        Position nxt = {cur.row + DR[d], cur.col + DC[d]};
        if (!is_open(maze, nxt.row, nxt.col)) continue;
        int nidx = nxt.row * maze->cols + nxt.col;
        if (visited[nidx]) continue;
        if (backtrack_dfs(maze, nxt, visited, path, depth + 1, path_length)) return 1;
    }
    return 0;
}

static int solve_backtracking(const Maze *maze) {
    const int n = maze->rows * maze->cols;
    unsigned char *visited = calloc((size_t)n, 1);
    Position *path = malloc((size_t)n * sizeof(*path));
    if (!visited || !path) {
        free(visited); free(path);
        return 0;
    }

    int length = 0;
    int found = backtrack_dfs(maze, maze->start, visited, path, 0, &length);
    if (found) print_path(path, length);
    else printf("No path found.\n");

    free(visited); free(path);
    return found;
}

static int solve_random_walk(const Maze *maze) {
    const int max_steps = maze->rows * maze->cols * 100;
    Position cur = maze->start;
    srand((unsigned)time(NULL));

    for (int step = 0; step < max_steps; ++step) {
        if (same_position(cur, maze->exit)) {
            printf("Exit reached after %d random-walk steps.\n", step);
            return 1;
        }

        Position candidates[4];
        int directions[4];
        int count = 0;
        for (int d = 0; d < 4; ++d) {
            Position nxt = {cur.row + DR[d], cur.col + DC[d]};
            if (is_open(maze, nxt.row, nxt.col)) {
                candidates[count] = nxt;
                directions[count] = d;
                ++count;
            }
        }
        if (count == 0) break;
        int choice = rand() % count;
        printf("%s ", DIR_NAME[directions[choice]]);
        cur = candidates[choice];
    }
    putchar('\n');
    printf("Random walk did not reach the exit within the step limit.\n");
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <random|backtrack|bfs> <maze.txt>\n", argv[0]);
        return EXIT_FAILURE;
    }

    Maze maze = {0};
    if (!load_maze(argv[2], &maze)) return EXIT_FAILURE;

    int success = 0;
    if (strcmp(argv[1], "random") == 0) success = solve_random_walk(&maze);
    else if (strcmp(argv[1], "backtrack") == 0) success = solve_backtracking(&maze);
    else if (strcmp(argv[1], "bfs") == 0) success = solve_bfs(&maze);
    else fprintf(stderr, "Unknown method '%s'.\n", argv[1]);

    free_maze(&maze);
    return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
