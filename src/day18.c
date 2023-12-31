#include "baz.h"

typedef struct {
  i64 x;
  i64 y;
} Pos;

static void solve(Span input) {
  SpanSplitIterator line_it = Span_split_lines(input);

  Pos current1 = {.x = 0, .y = 0};
  Pos previous1 = current1;

  usize perimeter1 = 0;
  i64 total1 = 0;

  Pos current2 = {.x = 0, .y = 0};
  Pos previous2 = current2;

  usize perimeter2 = 0;
  i64 total2 = 0;

  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  while (line.valid) {

    usize dist1 =
        UNWRAP(Span_parse_u64(Span_slice(line.dat, 2, line.dat.len), 10)).fst;
    perimeter1 += dist1;

    switch (line.dat.dat[0]) {
    case 'U':
      current1.y -= (i64)dist1;
      break;
    case 'R':
      current1.x += (i64)dist1;
      break;
    case 'D':
      current1.y += (i64)dist1;
      break;
    case 'L':
      current1.x -= (i64)dist1;
      break;
    default:
      panic("Unexpected\n");
    }

    total1 +=
        (i64)current1.y * (i64)previous1.x - (i64)current1.x * (i64)previous1.y;

    previous1 = current1;

    usize col =
        UNWRAP(Span_parse_u64(UNWRAP(Span_split_on('#', line.dat)).snd, 16))
            .fst;
    usize dist2 = col >> 4;
    perimeter2 += dist2;

    switch (col & 0b11) {
    case 0:
      current2.y -= (i64)dist2;
      break;
    case 1:
      current2.x += (i64)dist2;
      break;
    case 2:
      current2.y += (i64)dist2;
      break;
    case 3:
      current2.x -= (i64)dist2;
      break;
    default:
      panic("Unexpected\n");
    }

    total2 +=
        (i64)current2.y * (i64)previous2.x - (i64)current2.x * (i64)previous2.y;

    previous2 = current2;

    line = SpanSplitIterator_next(&line_it);
  }

  // shoelace algorithm
  usize area1 = (usize)ABS_DIFF(i64, total1, 0) / 2;
  usize area2 = (usize)ABS_DIFF(i64, total2, 0) / 2;

  // pick's theorem
  usize part1 = area1 + (perimeter1 / 2) + 1;
  usize part2 = area2 + (perimeter2 / 2) + 1;

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example = Span_from_str("R 6 (#70c710)\n"
                               "D 5 (#0dc571)\n"
                               "L 2 (#5713f0)\n"
                               "D 2 (#d2c081)\n"
                               "R 2 (#59c680)\n"
                               "D 2 (#411b91)\n"
                               "L 5 (#8ceee2)\n"
                               "U 2 (#caa173)\n"
                               "L 1 (#1b58a2)\n"
                               "U 2 (#caa171)\n"
                               "R 2 (#7807d2)\n"
                               "U 3 (#a77fa3)\n"
                               "L 2 (#015232)\n"
                               "U 2 (#7a21e3)\n");
  solve(example);

  Span input = Span_from_file("inputs/day18.txt");
  solve(input);

  return 0;
}
