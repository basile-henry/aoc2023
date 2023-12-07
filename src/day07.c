#include "baz.h"

typedef u8 Card; // 2-14 greater is better
typedef u8 Type; // 0-6 greater is better

typedef struct {
  Type type;
  Card cards[5];
} Hand;

typedef struct {
  Hand hand;
  u64 bid;
} Play;

static int Hand_cmp(const Hand *a, const Hand *b) {
  int cmp = u8_cmp(&a->type, &b->type);
  if (cmp != 0) {
    return cmp;
  }

  return memcmp(a->cards, b->cards, 5);
}

static int Play_cmp(const Play *a, const Play *b) {
  // We want a Min-Heap so negate
  return -Hand_cmp(&a->hand, &b->hand);
}

define_array(U8Array, u8, 8);

U8Array five_of_a_kind = {
    .len = 1,
    .dat = {5},
};
U8Array four_of_a_kind = {
    .len = 2,
    .dat = {1, 4},
};
U8Array full_house = {
    .len = 2,
    .dat = {2, 3},
};
U8Array three_of_a_kind = {
    .len = 3,
    .dat = {1, 1, 3},
};
U8Array two_pairs = {
    .len = 3,
    .dat = {1, 2, 2},
};
U8Array one_pair = {
    .len = 4,
    .dat = {1, 1, 1, 2},
};
U8Array high_card = {
    .len = 5,
    .dat = {1, 1, 1, 1, 1},
};

static bool U8Array_eq(const U8Array *a, const U8Array *b) {
  return a->len == b->len && memcmp(&a->dat, &b->dat, a->len) == 0;
}

static Hand Hand_parse(Span x, bool joker) {
  assert(x.len == 5);
  Hand hand = {0};
  u8 occurences[15] = {0};

  for (usize i = 0; i < 5; i++) {
    u8 c = x.dat[i];

    switch (c) {
    case 'A':
      hand.cards[i] = 14;
      break;
    case 'K':
      hand.cards[i] = 13;
      break;
    case 'Q':
      hand.cards[i] = 12;
      break;
    case 'J':
      hand.cards[i] = joker ? 1 : 11;
      break;
    case 'T':
      hand.cards[i] = 10;
      break;
    default:
      hand.cards[i] = (u8)from_digit(c, 10);
    }

    occurences[hand.cards[i]]++;
  }

  if (joker && occurences[1] > 0) {
    // Move joker occurences to the highest
    u8 j_occ = occurences[1];
    occurences[1] = 0;

    u8 max_v = 0;
    usize max_i = 0;
    for (usize i = 0; i < 15; i++) {
      if (occurences[i] > max_v) {
        max_i = i;
        max_v = occurences[i];
      }
    }

    occurences[max_i] += j_occ;
  }

  U8Array sorted_occurences = {0};

  for (usize i = 0; i < 15; i++) {
    if (occurences[i] > 0) {
      usize j = U8Array_bsearch(&sorted_occurences, &occurences[i], u8_cmp);
      U8Array_insert(&sorted_occurences, j, occurences[i]);
    }
  }

  if (U8Array_eq(&sorted_occurences, &high_card)) {
    hand.type = 0;
  } else if (U8Array_eq(&sorted_occurences, &one_pair)) {
    hand.type = 1;
  } else if (U8Array_eq(&sorted_occurences, &two_pairs)) {
    hand.type = 2;
  } else if (U8Array_eq(&sorted_occurences, &three_of_a_kind)) {
    hand.type = 3;
  } else if (U8Array_eq(&sorted_occurences, &full_house)) {
    hand.type = 4;
  } else if (U8Array_eq(&sorted_occurences, &four_of_a_kind)) {
    hand.type = 5;
  } else if (U8Array_eq(&sorted_occurences, &five_of_a_kind)) {
    hand.type = 6;
  } else {
    panic("Unexpected");
  }

  return hand;
}

static Play Play_parse(Span line, bool joker) {
  SpanSplitOn res = Span_split_on(' ', line);
  assert(res.valid);

  Hand hand = Hand_parse(res.dat.fst, joker);
  u64 bid = UNWRAP(Span_parse_u64(res.dat.snd, 10)).fst;

  return (Play){.hand = hand, .bid = bid};
}

define_binary_heap(PlaySet, Play, 1024, Play_cmp);

static void solve(Span input) {
  PlaySet play_set = {0};
  PlaySet play_set_joker = {0};

  SpanSplitIterator line_it = Span_split_lines(input);

  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  while (line.valid) {
    Play play = Play_parse(line.dat, false);
    PlaySet_insert(&play_set, play);

    Play play_joker = Play_parse(line.dat, true);
    PlaySet_insert(&play_set_joker, play_joker);

    line = SpanSplitIterator_next(&line_it);
  }

  usize part1 = 0;
  {
    usize rank = 1;
    PlaySetExtract play = PlaySet_extract(&play_set);
    while (play.valid) {
      part1 += rank * play.dat.bid;

      rank++;
      play = PlaySet_extract(&play_set);
    }
  }

  usize part2 = 0;
  {
    usize rank = 1;
    PlaySetExtract play = PlaySet_extract(&play_set_joker);
    while (play.valid) {
      part2 += rank * play.dat.bid;

      rank++;
      play = PlaySet_extract(&play_set_joker);
    }
  }

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example = Span_from_str("32T3K 765\n"
                               "T55J5 684\n"
                               "KK677 28\n"
                               "KTJJT 220\n"
                               "QQQJA 483\n");
  solve(example);

  Span input = Span_from_file("inputs/day07.txt");
  solve(input);

  return 0;
}
