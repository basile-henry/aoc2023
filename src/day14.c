#include "baz.h"

typedef struct {
  u8 x;
  u8 y;
} Pos;

static Hash Pos_hash(const Pos *pos) {
  FxHasher hasher = {0};
  FxHasher_add(&hasher, pos->x);
  FxHasher_add(&hasher, pos->y);
  return hasher;
}

static bool Pos_eq(const Pos *a, const Pos *b) {
  return a->x == b->x && a->y == b->y;
}

define_hash_map(Squares, Pos, u8, (2 * 1024), Pos_hash, Pos_eq);
define_array(RoundsFlat, Pos,
             (4 * 1024)); // Invariant: sorted from top to bottom
define_hash_map(RoundsSet, Pos, u8, (4 * 1024), Pos_hash, Pos_eq);

typedef struct {
  RoundsFlat flat;
  RoundsSet set;
} Rounds;

static void Rounds_clear(Rounds *rounds) {
  rounds->flat.len = 0;
  memset(&rounds->set, 0, sizeof(RoundsSet));
}

static void Rounds_push(Rounds *rounds, Pos pos) {
  RoundsFlat_push(&rounds->flat, pos);
  RoundsSet_insert(&rounds->set, pos, 0);
}

typedef T2(Hash, usize) RoundsSummary;

static bool RoundsSummary_eq(const RoundsSummary *a, const RoundsSummary *b) {
  return a->fst == b->fst && a->snd == b->snd;
}

define_array(History, RoundsSummary, 256);

unused static void Rounds_print(const Rounds *rounds, const Squares *squares,
                                usize dims) {
  for (u8 y = 0; y < dims; y++) {
    for (u8 x = 0; x < dims; x++) {
      Pos pos = {.x = x, .y = y};
      if (RoundsSet_contains(&rounds->set, &pos)) {
        putchar('O');
      } else if (Squares_contains(squares, &pos)) {
        if (pos.x == 33) {
          putchar('H');
        } else {
          putchar('#');
        }
      } else {
        putchar('.');
      }
    }
    putchar('\n');
  }
  putchar('\n');
}

static void Rounds_tilt_north(const Rounds *rounds, Rounds *moved,
                              const Squares *squares, usize width) {
  Rounds_clear(moved);

  u8 top_most[128] = {0};
  assert(width <= 128);

  for (usize i = 0; i < rounds->flat.len; i++) {
    Pos pos = rounds->flat.dat[i];

    while (pos.y > top_most[pos.x]) {
      if (Squares_contains(squares, &pos)) {
        do {
          pos.y++;
        } while (Squares_contains(squares, &pos));
        break;
      }

      pos.y--;
    }

    while (Squares_contains(squares, &pos)) {
      pos.y++;
    }

    top_most[pos.x] = pos.y + 1;
    Rounds_push(moved, pos);
  }
}

// Rotate so that west becomes north
static void Rounds_rotate(const Rounds *rounds, Rounds *moved, usize dims) {
  Rounds_clear(moved);

  // Make sure to keep the invariant that moved are sorted from top to bottom
  for (u8 y = 0; y < (u8)dims; y++) {
    for (u8 x = 0; x < (u8)dims; x++) {
      Pos pos = {.x = x, .y = y};
      Pos rotated = {
          .x = (u8)(dims - 1 - y),
          .y = x,
      };
      if (RoundsSet_contains(&rounds->set, &pos)) {
        Rounds_push(moved, rotated);
      }
    }
  }
}

// Rotate so that west becomes north
static void Squares_rotate(const Squares *rounds, Squares *moved, usize dims) {
  for (u8 y = 0; y < (u8)dims; y++) {
    for (u8 x = 0; x < (u8)dims; x++) {
      Pos pos = {.x = x, .y = y};
      Pos rotated = {
          .x = (u8)(dims - 1 - y),
          .y = x,
      };
      if (Squares_contains(rounds, &pos)) {
        Squares_insert(moved, rotated, 0);
      }
    }
  }
}

static usize Rounds_score(const Rounds *rounds, usize height) {
  usize score = 0;
  for (usize i = 0; i < rounds->flat.len; i++) {
    Pos pos = rounds->flat.dat[i];
    usize round_score = height - (usize)pos.y;
    score += round_score;
  }

  return score;
}

static RoundsSummary Rounds_summary(const Rounds *rounds, usize height) {
  FxHasher hasher = {0};
  for (usize i = 0; i < rounds->flat.len; i++) {
    Pos pos = rounds->flat.dat[i];
    FxHasher_add(&hasher, (usize)pos.x | (((usize)pos.y) << 8));
  }

  return (RoundsSummary){
      .fst = hasher,
      .snd = Rounds_score(rounds, height),
  };
}

static void solve(Span input) {
  SpanSplitIterator line_it = Span_split_lines(input);

  Squares squares = {0};
  Rounds rounds = {0};

  usize part1 = 0;

  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  u8 y = 0;
  usize dims = 0; // Assume square platform

  while (line.valid) {
    if (y == 0) {
      dims = line.dat.len; // Assume square platform
    }

    for (usize i = 0; i < line.dat.len; i++) {
      Pos pos = {.x = (u8)i, .y = y};

      if (line.dat.dat[i] == 'O') {
        Rounds_push(&rounds, pos);
      } else if (line.dat.dat[i] == '#') {
        Squares_insert(&squares, pos, 0);
      } else {
        assert(line.dat.dat[i] == '.');
      }
    }

    line = SpanSplitIterator_next(&line_it);
    y++;
  }

  Rounds moved = {0};
  Rounds_score(&rounds, dims);

  Rounds_tilt_north(&rounds, &moved, &squares, dims);
  part1 = Rounds_score(&moved, dims);

  Squares east = {0};
  Squares_rotate(&squares, &east, dims);
  Squares south = {0};
  Squares_rotate(&east, &south, dims);
  Squares west = {0};
  Squares_rotate(&south, &west, dims);

  History history = {0};

  usize cycle = 0;
  usize part2 = 0;
  while (true) {
    Rounds_tilt_north(&rounds, &moved, &squares, dims);

    Rounds_rotate(&moved, &rounds, dims);
    Rounds_tilt_north(&rounds, &moved, &east, dims);

    Rounds_rotate(&moved, &rounds, dims);
    Rounds_tilt_north(&rounds, &moved, &south, dims);

    Rounds_rotate(&moved, &rounds, dims);
    Rounds_tilt_north(&rounds, &moved, &west, dims);

    Rounds_rotate(&moved, &rounds, dims);

    RoundsSummary summary = Rounds_summary(&rounds, dims);

    HistoryLookup res =
        History_linear_lookup(&history, &summary, RoundsSummary_eq);
    if (res.valid) {
      usize first = res.dat;

      usize equiv = first + ((1000000000 - 1 - first) % (cycle - first));
      part2 = history.dat[equiv].snd;
      break;
    }

    History_push(&history, summary);

    cycle++;
  }

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example = Span_from_str("O....#....\n"
                               "O.OO#....#\n"
                               ".....##...\n"
                               "OO.#O....O\n"
                               ".O.....O#.\n"
                               "O.#..O.#.#\n"
                               "..O..#O..O\n"
                               ".......O..\n"
                               "#....###..\n"
                               "#OO..#....\n");
  solve(example);

  Span input = Span_from_file("inputs/day14.txt");
  solve(input);

  return 0;
}
