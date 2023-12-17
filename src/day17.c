#include "baz.h"

define_array(Row, u8, 256);
define_array(Grid, Row, 256);

typedef struct {
  u8 x;
  u8 y;
} Pos;

static inline bool Pos_eq(const Pos *a, const Pos *b) {
  return a->x == b->x && a->y == b->y;
}

#define UP 0
#define RIGHT 1
#define DOWN 2
#define LEFT 3

// No checks for out of bound
static inline Pos Pos_move(Pos p, u8 dir) {
  switch (dir) {
  case UP:
    return (Pos){.x = p.x, .y = p.y - 1};
    break;
  case RIGHT:
    return (Pos){.x = p.x + 1, .y = p.y};
    break;
  case DOWN:
    return (Pos){.x = p.x, .y = p.y + 1};
    break;
  case LEFT:
    return (Pos){.x = p.x - 1, .y = p.y};
    break;
  default:
    panic("Unexpected\n");
  }
}

typedef struct {
  usize heat_loss;
  Pos pos;
  Pos goal;
  u8 straight; // 0, 1, 2
  u8 dir;      // 0 - UP; 1 - RIGHT; 2 - DOWN; 3 - LEFT
} State;

static inline u8 Dir_opposite(u8 dir) {
  switch (dir) {
  case UP:
    return DOWN;
    break;
  case RIGHT:
    return LEFT;
    break;
  case DOWN:
    return UP;
    break;
  case LEFT:
    return RIGHT;
    break;
  default:
    panic("Unexpected\n");
  }
}

static inline usize State_manhattan(const State *s) {
  return ABS_DIFF(usize, s->pos.x, s->goal.x) +
         ABS_DIFF(usize, s->pos.y, s->goal.y);
}

static int State_cmp(const State *a, const State *b) {
  int cmp = usize_cmp(&a->heat_loss, &b->heat_loss);
  if (cmp != 0) {
    return -cmp;
  }

  usize manhattan_a = State_manhattan(a);
  usize manhattan_b = State_manhattan(b);
  return -usize_cmp(&manhattan_a, &manhattan_b);
}

typedef struct {
  Pos pos;
  u8 straight;
  u8 dir;
} Entry;

static Hash Entry_hash(const Entry *e) {
  FxHasher hasher = {0};
  FxHasher_add(&hasher, e->pos.x);
  FxHasher_add(&hasher, e->pos.y);
  FxHasher_add(&hasher, e->straight);
  FxHasher_add(&hasher, e->dir);
  return hasher;
}

static inline bool Entry_eq(const Entry *a, const Entry *b) {
  return Pos_eq(&a->pos, &b->pos) && a->straight == b->straight &&
         a->dir == b->dir;
}

define_binary_heap(PriorityQueue, State, (8 * 1024), State_cmp);
define_hash_map(Cache, Entry, usize, (1024 * 1024), Entry_hash, Entry_eq);
static Cache cache;

#ifdef DEBUG
typedef T2(Entry, u8) PosDir;

define_hash_map(Prevs, Entry, PosDir, (1024 * 1024), Entry_hash, Entry_eq);
static Prevs prevs;
#endif // DEBUG

static void solve(Span input, u8 straight_min, u8 straight_max) {
  SpanSplitIterator line_it = Span_split_lines(input);

  Grid grid = {0};
  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  while (line.valid) {
    for (usize i = 0; i < line.dat.len; i++) {
      Row_push(&grid.dat[grid.len], (u8)from_digit(line.dat.dat[i], 10));
    }
    grid.len++;
    line = SpanSplitIterator_next(&line_it);
  }

  usize best_heat_loss = 0;
  {
    PriorityQueue pq = {0};
    Cache *c = &cache;
    memset(c, 0, sizeof(Cache));
#ifdef DEBUG
    Prevs *p = &prevs;
    memset(p, 0, sizeof(Prevs));
#endif // DEBUG

    Pos start = {
        .x = 0,
        .y = 0,
    };
    Pos goal = {
        .x = (u8)grid.dat[0].len - 1,
        .y = (u8)grid.len - 1,
    };

    PriorityQueue_insert(&pq, (State){
                                 .heat_loss = 0,
                                 .pos = start,
                                 .goal = goal,
                                 .straight = 0,
                                 .dir = RIGHT,
                             });
    PriorityQueue_insert(&pq, (State){
                                 .heat_loss = 0,
                                 .pos = start,
                                 .goal = goal,
                                 .straight = 0,
                                 .dir = DOWN,
                             });
    Cache_insert(c,
                 (Entry){
                     .pos = start,
                     .straight = 0,
                     .dir = RIGHT,
                 },
                 0);
    Cache_insert(c,
                 (Entry){
                     .pos = start,
                     .straight = 0,
                     .dir = DOWN,
                 },
                 0);

    State best = {0};
    bool found_solution = false;

    while (pq.len > 0) {
      State current = UNWRAP(PriorityQueue_extract(&pq));

      if (Pos_eq(&current.pos, &goal) && current.straight >= straight_min) {
        if (!found_solution || best.heat_loss > current.heat_loss) {
          best = current;
          found_solution = true;
        }
        continue;
      }

      for (u8 dir = 0; dir < 4; dir++) {
        // Skip going backwards
        if (Dir_opposite(current.dir) == dir) {
          continue;
        }

        // Skip going outside
        if (current.pos.x == 0 && dir == LEFT) {
          continue;
        }
        if (current.pos.y == 0 && dir == UP) {
          continue;
        }
        if (current.pos.x == goal.x && dir == RIGHT) {
          continue;
        }
        if (current.pos.y == goal.y && dir == DOWN) {
          continue;
        }

        // Skip going straight for too long
        if (current.straight == straight_max && current.dir == dir) {
          continue;
        }

        // Skip going straight for too short
        if (current.straight < straight_min && current.dir != dir) {
          continue;
        }

        Pos next = Pos_move(current.pos, dir);
        u8 straight = current.dir == dir ? current.straight + 1 : 1;

        usize new_heat_loss =
            current.heat_loss + (usize)grid.dat[next.y].dat[next.x];

        Entry entry = {
            .pos = next,
            .straight = straight,
            .dir = dir,
        };
        usize *next_heat_loss =
            Cache_insert_modify(c, entry, (usize)UINT32_MAX);

        if (*next_heat_loss == UINT32_MAX || new_heat_loss < *next_heat_loss) {
          *next_heat_loss = new_heat_loss;
          State next_state = {
              .heat_loss = new_heat_loss,
              .pos = next,
              .goal = goal,
              .straight = straight,
              .dir = dir,
          };
          PriorityQueue_insert(&pq, next_state);
#ifdef DEBUG
          Prevs_insert(p, entry,
                       (PosDir){.fst = (Entry){.pos = current.pos,
                                               .straight = current.straight,
                                               .dir = current.dir},
                                .snd = dir});
#endif // DEBUG
        }
      }
    }

    best_heat_loss = best.heat_loss;

#ifdef DEBUG
    Grid solved;
    memcpy(&solved, &grid, sizeof(Grid));
    printf2("Grid dims: %ux%u\n", solved.len, solved.dat[0].len);

    Entry trace = {
        .pos = best.pos,
        .straight = best.straight,
        .dir = best.dir,
    };

    while (!Pos_eq(&trace.pos, &start)) {
      PosDir x = *UNWRAP(Prevs_lookup(p, &trace));
      solved.dat[trace.pos.y].dat[trace.pos.x] = 10 + x.snd;
      trace = x.fst;
    }

    for (usize y = 0; y < solved.len; y++) {
      for (usize x = 0; x < solved.dat[0].len; x++) {
        u8 z = solved.dat[y].dat[x];
        if (z >= 10) {
          switch (z - 10) {
          case UP:
            putstr("\033[31;1m^\033[0m");
            break;
          case RIGHT:
            putstr("\033[31;1m>\033[0m");
            break;
          case DOWN:
            putstr("\033[31;1mv\033[0m");
            break;
          case LEFT:
            putstr("\033[31;1m<\033[0m");
            break;
          default:
            panic("Unexpected\n");
          }
        } else {
          putu64(z);
        }
      }
      putchar('\n');
    }
#endif // DEBUG
  }

  printf1("%u\n", best_heat_loss);
}

int main(void) {
  Span example = Span_from_str("2413432311323\n"
                               "3215453535623\n"
                               "3255245654254\n"
                               "3446585845452\n"
                               "4546657867536\n"
                               "1438598798454\n"
                               "4457876987766\n"
                               "3637877979653\n"
                               "4654967986887\n"
                               "4564679986453\n"
                               "1224686865563\n"
                               "2546548887735\n"
                               "4322674655533\n");
  solve(example, 1, 3);
  solve(example, 4, 10);

  Span example2 = Span_from_str("111111111111\n"
                                "999999999991\n"
                                "999999999991\n"
                                "999999999991\n"
                                "999999999991\n");
  solve(example2, 4, 10);

  Span input = Span_from_file("inputs/day17.txt");
  solve(input, 1, 3);
  solve(input, 4, 10);

  return 0;
}
