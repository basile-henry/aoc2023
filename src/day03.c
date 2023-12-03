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

define_hash_map(Parts, Pos, u32, 2048, Pos_hash, Pos_eq);

static bool is_symbol(u8 c) { return !is_digit(c, 10) && c != '.'; }

static usize index(usize line_len, Pos pos) {
  return (usize)pos.x + (line_len + 1) * (usize)pos.y;
}

typedef struct {
  Pos pos;
  u32 part;
} Part;

static Part read_part(Span input, usize line_len, Pos from) {
  u8 x = from.x;

  // Keep going left while it's a digit
  while (x > 0 &&
         is_digit(input.dat[index(line_len, (Pos){.x = x - 1, .y = from.y})],
                  10)) {
    x--;
  }

  Pos pos = {.x = x, .y = from.y};
  usize ix = index(line_len, pos);
  SpanParseU64 res = Span_parse_u64(Span_slice(input, ix, input.len), 10);
  assert(res.valid);

  return (Part){.pos = pos, .part = (u32)res.dat.fst};
}

typedef Option(Part) OptionPart;

static void solve(Span input) {
  Parts parts = {0};
  usize part1 = 0;
  usize part2 = 0;

  SpanSplitOn line = Span_split_on('\n', input);
  assert(line.valid);
  usize line_len = line.dat.fst.len;
  usize num_lines = input.len / (line_len + 1);

  for (u8 y = 0; y < (u8)num_lines; y++) {
    for (u8 x = 0; x < (u8)line_len; x++) {
      u8 symbol = input.dat[index(line_len, (Pos){.x = x, .y = y})];
      if (!is_symbol(symbol)) {
        continue;
      }

      OptionPart gear_part = {0};
      for (isize i = -1; i <= 1; i++) {
        if ((x == 0 && i == -1) || ((x == line_len - 1) && i == 1)) {
          continue;
        }

        for (isize j = -1; j <= 1; j++) {
          if ((y == 0 && j == -1) || ((y == num_lines - 1) && j == 1) ||
              (i == 0 && j == 0)) {
            continue;
          }

          Pos pos = {.x = (u8)((isize)x + i), .y = (u8)((isize)y + j)};
          if (is_digit(input.dat[index(line_len, pos)], 10)) {
            Part part = read_part(input, line_len, pos);

            bool overwriting = Parts_insert(&parts, part.pos, part.part);

            if (!overwriting) {
              part1 += part.part;
            }

            if (!overwriting && symbol == '*') {
              if (!gear_part.valid) {
                gear_part.valid = true;
                gear_part.dat = part;
              } else if (!Pos_eq(&gear_part.dat.pos, &pos)) {
                u32 gear_ratio = gear_part.dat.part * part.part;
                part2 += gear_ratio;
              }
            }
          }
        }
      }
    }
  }

  String out = {0};
  String_push_str(&out, "part 1: ");
  String_push_u64(&out, part1, 10);
  String_push_str(&out, " | part 2: ");
  String_push_u64(&out, part2, 10);
  String_println(&out);
}

int main(void) {
  Span example = Span_from_str("467..114..\n"
                               "...*......\n"
                               "..35..633.\n"
                               "......#...\n"
                               "617*......\n"
                               ".....+.58.\n"
                               "..592.....\n"
                               "......755.\n"
                               "...$.*....\n"
                               ".664.598..\n");
  solve(example);

  Span input = Span_from_file("inputs/day03.txt");
  solve(input);

  return 0;
}
