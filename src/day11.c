#include "baz.h"

typedef struct {
  u64 x;
  u64 y;
} Pos;

static inline Pos Pos_add(Pos a, Pos b) {
  return (Pos){.x = a.x + b.x, .y = a.y + b.y};
}

static inline Pos Pos_mul(Pos a, usize n) {
  return (Pos){.x = n * a.x, .y = n * a.y};
}

static usize manhattan_distance(Pos a, Pos b) {
  return ABS_DIFF(usize, (usize)a.x, (usize)b.x) +
         ABS_DIFF(usize, (usize)a.y, (usize)b.y);
}

define_array(Galaxies, Pos, 512);
define_bit_set(GalaxySet, u64, 3);

static void solve(Span input) {
  SpanSplitIterator line_it = Span_split_lines(input);

  Galaxies galaxies = {0};
  Galaxies expanse = {0};

  GalaxySet columns = {0};
  GalaxySet rows = {0};

  u64 y = 0;
  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  while (line.valid) {
    for (usize i = 0; i < line.dat.len; i++) {
      if (line.dat.dat[i] == '#') {
        u64 x = (u64)i;
        Galaxies_push(&galaxies, (Pos){
                                     .x = x,
                                     .y = y,
                                 });

        GalaxySet_insert(&columns, x);
        GalaxySet_insert(&rows, y);
      }
    }

    y++;
    line = SpanSplitIterator_next(&line_it);
  }

  // Universe expanse
  for (usize i = 0; i < galaxies.len; i++) {
    Pos p = galaxies.dat[i];

    GalaxySet x_mask = {0};
    GalaxySet_set_many(&x_mask, p.x);

    GalaxySet y_mask = {0};
    GalaxySet_set_many(&y_mask, p.y);

    usize x_expansion =
        p.x - GalaxySet_count(GalaxySet_intersection(x_mask, columns));
    usize y_expansion =
        p.y - GalaxySet_count(GalaxySet_intersection(y_mask, rows));

    Galaxies_push(&expanse, (Pos){.x = x_expansion, .y = y_expansion});
  }

  usize part1 = 0;
  usize part2 = 0;
  for (usize i = 0; i < galaxies.len - 1; i++) {
    for (usize j = i + 1; j < galaxies.len; j++) {
      {
        Pos p_i = Pos_add(galaxies.dat[i], expanse.dat[i]);
        Pos p_j = Pos_add(galaxies.dat[j], expanse.dat[j]);

        part1 += manhattan_distance(p_i, p_j);
      }

      {
        Pos p_i = Pos_add(galaxies.dat[i], Pos_mul(expanse.dat[i], 999999));
        Pos p_j = Pos_add(galaxies.dat[j], Pos_mul(expanse.dat[j], 999999));

        part2 += manhattan_distance(p_i, p_j);
      }
    }
  }

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example = Span_from_str("...#......\n"
                               ".......#..\n"
                               "#.........\n"
                               "..........\n"
                               "......#...\n"
                               ".#........\n"
                               ".........#\n"
                               "..........\n"
                               ".......#..\n"
                               "#...#.....\n");
  solve(example);

  Span input = Span_from_file("inputs/day11.txt");
  solve(input);

  return 0;
}
