#include "baz.h"

define_array(Pattern, u32, 32);

typedef Option(usize) Reflection;
static Reflection Pattern_reflection(const Pattern *pat, usize discard) {
  isize len = (isize)pat->len;

  isize min_overlap = 2;
  isize bound = len - min_overlap;

  for (isize offset = -bound; offset <= bound; offset += 2) {
    bool reflect = true;

    for (isize i = 0; i < len; i++) {
      isize j = i + offset;
      isize k = (len + offset) - 1 - j;

      if (j < 0 || j >= len) {
        continue;
      }

      assert(k >= 0 && k < len);

      if (pat->dat[j] != pat->dat[k]) {
        reflect = false;
        break;
      }
    }

    if (reflect) {
      isize reflect = offset + ((len - offset) / 2);
      assert(reflect >= 0 && reflect < len);

      if ((usize)reflect != discard) {
        return (Reflection){
            .valid = true,
            .dat = (usize)reflect,
        };
      }
    }
  }

  return (Reflection){
      .valid = false,
  };
}

typedef struct {
  Pattern rows;
  Pattern columns;
} Patterns;

static void Patterns_clear(Patterns *pat) {
  pat->rows.len = 0;
  pat->columns.len = 0;
}

static void Patterns_push(Patterns *pat, Span line) {
  if (pat->columns.len == 0) {
    memset(&pat->columns, 0, sizeof(pat->columns));
    pat->columns.len = line.len;
  }
  assert(pat->columns.len == line.len);

  u16 row = 0;

  for (usize i = 0; i < line.len; i++) {
    if (line.dat[i] == '#') {
      row |= (1 << i);
      pat->columns.dat[i] |= (1 << pat->rows.len);
    } else {
      assert(line.dat[i] == '.');
    }
  }

  Pattern_push(&pat->rows, row);
}

typedef Option(usize) Summary;
static Summary Patterns_summarize(const Patterns *pat, usize discard) {
  Reflection cols_ref = Pattern_reflection(&pat->columns, discard);
  if (cols_ref.valid) {
    return (Summary){
        .valid = true,
        .dat = cols_ref.dat,
    };
  }

  Reflection rows_ref = Pattern_reflection(&pat->rows, discard / 100);
  if (rows_ref.valid) {
    return (Summary){
        .valid = true,
        .dat = 100 * rows_ref.dat,
    };
  }

  return (Summary){
      .valid = false,
  };
}

static usize Patterns_summarize_smudged(const Patterns *pat) {
  Patterns smuged = {0};
  Summary r = Patterns_summarize(pat, 0);
  assert(r.valid);

  for (usize i = 0; i < pat->rows.len; i++) {
    for (usize j = 0; j < pat->columns.len; j++) {
      memcpy(&smuged, pat, sizeof(Patterns));
      bool is_rock = (bool)((smuged.rows.dat[i] >> j) & 1);

      if (is_rock) {
        smuged.rows.dat[i] &= ~((u32)1 << j);
        smuged.columns.dat[j] &= ~((u32)1 << i);
      } else {
        smuged.rows.dat[i] |= ((u32)1 << j);
        smuged.columns.dat[j] |= ((u32)1 << i);
      }

      Summary s = Patterns_summarize(&smuged, r.dat);
      if (s.valid) {
        return s.dat;
      }
    }
  }

  panic("Unexpected\n");
}

static void solve(Span input) {
  SpanSplitIterator line_it = Span_split_lines(input);

  usize part1 = 0;
  usize part2 = 0;
  Patterns pat = {0};

  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  while (line.valid) {

    if (line.dat.len > 0) {
      Patterns_push(&pat, line.dat);
    } else {
      part1 += UNWRAP(Patterns_summarize(&pat, 0));
      part2 += Patterns_summarize_smudged(&pat);
      Patterns_clear(&pat);
    }

    line = SpanSplitIterator_next(&line_it);
  }

  part1 += UNWRAP(Patterns_summarize(&pat, 0));
  part2 += Patterns_summarize_smudged(&pat);

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example = Span_from_str("#.##..##.\n"
                               "..#.##.#.\n"
                               "##......#\n"
                               "##......#\n"
                               "..#.##.#.\n"
                               "..##..##.\n"
                               "#.#.##.#.\n"
                               "\n"
                               "#...##..#\n"
                               "#....#..#\n"
                               "..##..###\n"
                               "#####.##.\n"
                               "#####.##.\n"
                               "..##..###\n"
                               "#....#..#\n");
  solve(example);

  Span input = Span_from_file("inputs/day13.txt");
  solve(input);

  return 0;
}
