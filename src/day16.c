#include "baz.h"

typedef struct {
  u8 x;
  u8 y;
} Pos;

static inline usize Pos_index(Pos pos, u8 dim) {
  return (usize)pos.x + (((usize)dim + 1) * (usize)pos.y);
}

typedef struct {
  Pos pos;
  u8 dir;
} State;

static Hash State_hash(const State *state) {
  FxHasher hasher = {0};
  FxHasher_add(&hasher, (usize)state->dir);
  FxHasher_add(&hasher, (usize)state->pos.x);
  FxHasher_add(&hasher, (usize)state->pos.y);
  return hasher;
}

static bool State_eq(const State *a, const State *b) {
  return a->dir == b->dir && a->pos.x == b->pos.x && a->pos.y == b->pos.y;
}

typedef Option(State) NextState;
static NextState State_next(State state, u8 dim) {
  switch (state.dir) {
  case 0:
    // UP
    if (state.pos.y == 0) {
      return (NextState){.valid = false};
    }
    state.pos.y--;
    break;
  case 1:
    // RIGHT
    if (state.pos.x == dim - 1) {
      return (NextState){.valid = false};
    }
    state.pos.x++;
    break;
  case 2:
    // DOWN
    if (state.pos.y == dim - 1) {
      return (NextState){.valid = false};
    }
    state.pos.y++;
    break;
  case 3:
    // LEFT
    if (state.pos.x == 0) {
      return (NextState){.valid = false};
    }
    state.pos.x--;
    break;
  default:
    panic("Unexpected\n");
  }

  return (NextState){.valid = true, .dat = state};
}

define_array(ToVisit, State, 256);
define_hash_map(Visited, usize, u8, (16 * 1024), usize_hash, usize_eq);
define_hash_map(VisitedState, State, u8, (16 * 1024), State_hash, State_eq);

static void solve(Span input) {
  // Assume square
  u8 dim = (u8)UNWRAP(Span_split_on('\n', input)).fst.len;
  usize part1 = 0;
  usize part2 = 0;

  for (u8 o = 0; o < dim; o++) {
    for (u8 d = 0; d < 4; d++) {
      Visited visited = {0};
      VisitedState visited_state = {0};
      ToVisit to_visit = {0};

      State start = {
          .pos = {.x = 0, .y = 0},
          .dir = d,
      };
      switch (d) {
      case 0:
        // UP (start from bottom)
        start.pos.x = o;
        start.pos.y = dim - 1;
        break;
      case 1:
        // RIGHT (start from left)
        start.pos.y = o;
        break;
      case 2:
        // DOWN (start from top)
        start.pos.x = o;
        break;
      case 3:
        // LEFT (start from right)
        start.pos.x = dim - 1;
        start.pos.y = o;
        break;
      default:
        panic("Unexpected\n");
      }

      ToVisit_push(&to_visit, start);

      ToVisitPop current = ToVisit_pop(&to_visit);
      while (current.valid) {
        if (VisitedState_contains(&visited_state, &current.dat)) {
          current = ToVisit_pop(&to_visit);
          continue;
        }

        VisitedState_insert(&visited_state, current.dat, 0);
        Visited_insert(&visited, Pos_index(current.dat.pos, dim),
                       current.dat.dir);

        switch (input.dat[Pos_index(current.dat.pos, dim)]) {
        case '.': {
          // continue straight
          NextState next = State_next(current.dat, dim);
          if (next.valid) {
            ToVisit_push(&to_visit, next.dat);
          }
          break;
        }
        case '\\': {
          switch (current.dat.dir) {
          case 0:
            current.dat.dir = 3;
            break;
          case 1:
            current.dat.dir = 2;
            break;
          case 2:
            current.dat.dir = 1;
            break;
          case 3:
            current.dat.dir = 0;
            break;
          default:
            panic("Unexpected\n");
          }
          NextState next = State_next(current.dat, dim);
          if (next.valid) {
            ToVisit_push(&to_visit, next.dat);
          }
          break;
        }
        case '/':
          switch (current.dat.dir) {
          case 0:
            current.dat.dir = 1;
            break;
          case 1:
            current.dat.dir = 0;
            break;
          case 2:
            current.dat.dir = 3;
            break;
          case 3:
            current.dat.dir = 2;
            break;
          default:
            panic("Unexpected\n");
          }
          NextState next = State_next(current.dat, dim);
          if (next.valid) {
            ToVisit_push(&to_visit, next.dat);
          }
          break;
        case '|':
          if (current.dat.dir == 1 || current.dat.dir == 3) {
            current.dat.dir = 0;
            {
              NextState next = State_next(current.dat, dim);
              if (next.valid) {
                ToVisit_push(&to_visit, next.dat);
              }
            }
            current.dat.dir = 2;
            {
              NextState next = State_next(current.dat, dim);
              if (next.valid) {
                ToVisit_push(&to_visit, next.dat);
              }
            }
          } else {
            // continue straight
            NextState next = State_next(current.dat, dim);
            if (next.valid) {
              ToVisit_push(&to_visit, next.dat);
            }
          }
          break;
        case '-':
          if (current.dat.dir == 0 || current.dat.dir == 2) {
            current.dat.dir = 1;
            {
              NextState next = State_next(current.dat, dim);
              if (next.valid) {
                ToVisit_push(&to_visit, next.dat);
              }
            }
            current.dat.dir = 3;
            {
              NextState next = State_next(current.dat, dim);
              if (next.valid) {
                ToVisit_push(&to_visit, next.dat);
              }
            }
          } else {
            // continue straight
            NextState next = State_next(current.dat, dim);
            if (next.valid) {
              ToVisit_push(&to_visit, next.dat);
            }
          }
          break;
        default:
          panic("Unexpected\n");
        }

        current = ToVisit_pop(&to_visit);
      }

      if (o == 0 && d == 1) {
        part1 = visited.count;
      }

      part2 = visited.count > part2 ? visited.count : part2;
    }
  }

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example = Span_from_str(".|...\\....\n"
                               "|.-.\\.....\n"
                               ".....|-...\n"
                               "........|.\n"
                               "..........\n"
                               ".........\\\n"
                               "..../.\\\\..\n"
                               ".-.-/..|..\n"
                               ".|....-|.\\\n"
                               "..//.|....\n");

  solve(example);

  Span input = Span_from_file("inputs/day16.txt");
  solve(input);

  return 0;
}
