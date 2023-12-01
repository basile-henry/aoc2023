#include "baz.h"

void solve(Span data) {
  SpanSplitIterator line_it = Span_split_lines(data);
  u64 part1 = 0;
  u64 part2 = 0;

  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  while (line.valid) {
    assert(line.dat.len >= 2);

    u8 cal[2] = {0};
    u8 cal_2[2] = {0};

    bool found_first = false;
    bool found_first_2 = false;

    for (usize i = 0; i < line.dat.len; i++) {
      Span from_here = Span_slice(line.dat, i, line.dat.len);

      u8 c = line.dat.dat[i];
      u8 d = line.dat.dat[i];

      if (Span_starts_with_str(from_here, "one")) {
        d = '1';
      } else if (Span_starts_with_str(from_here, "two")) {
        d = '2';
      } else if (Span_starts_with_str(from_here, "three")) {
        d = '3';
      } else if (Span_starts_with_str(from_here, "four")) {
        d = '4';
      } else if (Span_starts_with_str(from_here, "five")) {
        d = '5';
      } else if (Span_starts_with_str(from_here, "six")) {
        d = '6';
      } else if (Span_starts_with_str(from_here, "seven")) {
        d = '7';
      } else if (Span_starts_with_str(from_here, "eight")) {
        d = '8';
      } else if (Span_starts_with_str(from_here, "nine")) {
        d = '9';
      }

      if (is_digit(c, 10)) {

        if (!found_first) {
          cal[0] = c;
          found_first = true;
        }

        cal[1] = c;
      }

      if (is_digit(d, 10)) {
        if (!found_first_2) {
          cal_2[0] = d;
          found_first_2 = true;
        }

        cal_2[1] = d;
      }
    }

    if (found_first) {
      part1 += from_digit(cal[0], 10) * 10 + from_digit(cal[1], 10);
    }
    if (found_first_2) {
      part2 += from_digit(cal_2[0], 10) * 10 + from_digit(cal_2[1], 10);
    }

    line = SpanSplitIterator_next(&line_it);
  }

  String out = {0};
  String_push_u64(&out, part1, 10);
  String_push_str(&out, " | ");
  String_push_u64(&out, part2, 10);
  String_println(&out);
}

int main(void) {
  Span example = Span_from_str("1abc2\n"
                               "pqr3stu8vwx\n"
                               "a1b2c3d4e5f\n"
                               "treb7uchet\n");
  solve(example);

  Span example2 = Span_from_str("two1nine\n"
                                "eightwothree\n"
                                "abcone2threexyz\n"
                                "xtwone3four\n"
                                "4nineeightseven2\n"
                                "zoneight234\n"
                                "7pqrstsixteen\n");

  solve(example2);

  Span input = Span_from_file("inputs/day01.txt");
  solve(input);

  return 0;
}
