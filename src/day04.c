#include "baz.h"

define_bit_set(Card, u64, 2);

static Card Card_parse(Span x) {
  Card card = {0};
  SpanSplitIterator word_it = Span_split_words(x);

  SpanSplitIteratorNext word = SpanSplitIterator_next(&word_it);
  while (word.valid) {
    if (word.dat.len == 0) {
      word = SpanSplitIterator_next(&word_it);
      continue;
    }

    Card_insert(&card, UNWRAP(Span_parse_u64(word.dat, 10)).fst);
    word = SpanSplitIterator_next(&word_it);
  }

  return card;
}

static void solve(Span input) {
  u64 part1 = 0;
  u64 part2 = 0;
  u64 card_copies[256] = {0};

  SpanSplitIterator line_it = Span_split_lines(input);

  usize ix = 0;
  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  while (line.valid) {
    Span both_span = UNWRAP(Span_split_on(':', line.dat)).snd;
    SpanSplitOn both = Span_split_on('|', both_span);
    assert(both.valid);

    Card winning = Card_parse(both.dat.fst);
    Card numbers = Card_parse(both.dat.snd);

    Card match = Card_intersection(winning, numbers);
    usize count = Card_count(match);
    part1 += ((u64)1 << count) >> 1;

    u64 num_cards = card_copies[ix] + 1;
    part2 += num_cards;
    ;

    for (usize i = ix + 1; i <= ix + count; i++) {
      card_copies[i] += num_cards;
    }

    line = SpanSplitIterator_next(&line_it);
    ix++;
  }

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example =
      Span_from_str("Card 1: 41 48 83 86 17 | 83 86  6 31 17  9 48 53\n"
                    "Card 2: 13 32 20 16 61 | 61 30 68 82 17 32 24 19\n"
                    "Card 3:  1 21 53 59 44 | 69 82 63 72 16 21 14  1\n"
                    "Card 4: 41 92 73 84 69 | 59 84 76 51 58  5 54 83\n"
                    "Card 5: 87 83 26 28 32 | 88 30 70 12 93 22 82 36\n"
                    "Card 6: 31 18 13 56 72 | 74 77 10 23 35 67 36 11\n");
  solve(example);

  Span input = Span_from_file("inputs/day04.txt");
  solve(input);

  return 0;
}
