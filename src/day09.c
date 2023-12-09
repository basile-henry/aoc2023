#include "baz.h"

define_array(Nums, i64, 32);
define_array(Stack, Nums, 32);

static inline void Stack_reset(Stack *stack) {
  for (usize i = 0; i < stack->len; i++) {
    stack->dat[i].len = 0;
  }

  stack->len = 0;
}

static void Nums_parse(Nums *nums, Span line) {
  SpanParseI64 res = Span_parse_i64(line, 10);

  while (res.valid) {
    Nums_push(nums, res.dat.fst);

    if (res.dat.snd.len == 0) {
      return;
    }

    line = Span_slice(res.dat.snd, 1, res.dat.snd.len);
    res = Span_parse_i64(line, 10);
  }
}

typedef struct {
  i64 before;
  i64 after;
} Predict;

static Predict predict(Stack *stack) {
  assert(stack->len < 32);

  Nums *in = &stack->dat[stack->len - 1];
  Nums *out = &stack->dat[stack->len];
  stack->len++;

  bool all_zeros = true;
  i64 first = in->dat[0];
  i64 last = in->dat[in->len - 1];

  for (usize i = 0; i < in->len - 1; i++) {
    i64 diff = in->dat[i + 1] - in->dat[i];
    if (diff != 0) {
      all_zeros = false;
    }

    Nums_push(out, diff);
  }

  if (all_zeros) {
    return (Predict){.before = first, .after = last};
  } else {
    Predict p = predict(stack);
    return (Predict){
        .before = first - p.before,
        .after = last + p.after,
    };
  }
}

static void solve(Span input) {
  SpanSplitIterator line_it = Span_split_lines(input);

  i64 part1 = 0;
  i64 part2 = 0;
  Stack stack = {0};

  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  while (line.valid) {
    Stack_reset(&stack);
    Nums_parse(&stack.dat[stack.len], line.dat);
    stack.len++;

    Predict p = predict(&stack);
    part1 += p.after;
    part2 += p.before;

    line = SpanSplitIterator_next(&line_it);
  }

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example = Span_from_str("0 3 6 9 12 15\n"
                               "1 3 6 10 15 21\n"
                               "10 13 16 21 30 45\n");
  solve(example);

  Span input = Span_from_file("inputs/day09.txt");
  solve(input);

  return 0;
}
