#include "baz.h"

typedef struct {
  u8 red;
  u8 green;
  u8 blue;
} Bag;

define_array(GameSets, Bag, 8);

typedef struct {
  u64 id;
  GameSets sets;
} Game;

Bag Bag_parse(Span span) {
  Bag bag = {0};

  SpanSplitIterator word_it = Span_split_words(span);

  SpanSplitIteratorNext word = SpanSplitIterator_next(&word_it);
  while (word.valid) {
    if (word.dat.len == 0) {
      word = SpanSplitIterator_next(&word_it);
      continue;
    }

    SpanParseU64 res = Span_parse_u64(word.dat, 10);
    assert(res.valid);

    word = SpanSplitIterator_next(&word_it);
    assert(word.valid);

    if (Span_starts_with_str(word.dat, "red")) {
      bag.red = (u8)res.dat.fst;
    } else if (Span_starts_with_str(word.dat, "green")) {
      bag.green = (u8)res.dat.fst;
    } else if (Span_starts_with_str(word.dat, "blue")) {
      bag.blue = (u8)res.dat.fst;
    } else {
      panic("Unexpected");
    }

    word = SpanSplitIterator_next(&word_it);
  }

  return bag;
}

void Game_parse(Game *out, Span line) {
  // Game ID
  SpanSplitOn colon = Span_split_on(':', line);
  assert(colon.valid);

  SpanSplitOn space = Span_split_on(' ', colon.dat.fst);
  assert(space.valid);

  SpanParseU64 id = Span_parse_u64(space.dat.snd, 10);
  assert(id.valid);
  out->id = id.dat.fst;

  // Game sets
  SpanSplitIterator set_it = {
      .rest = colon.dat.snd,
      .sep = ';',
  };

  SpanSplitIteratorNext set = SpanSplitIterator_next(&set_it);
  while (set.valid) {
    Bag bag = Bag_parse(set.dat);
    GameSets_push(&out->sets, bag);

    set = SpanSplitIterator_next(&set_it);
  }
}

bool Game_possible(const Game *game, Bag bag) {
  for (usize i = 0; i < game->sets.len; i++) {
    Bag set = game->sets.dat[i];
    if (set.red > bag.red || set.green > bag.green || set.blue > bag.blue) {
      return false;
    }
  }

  return true;
}

Bag Game_minimum(const Game *game) {
  Bag bag = {0};

  for (usize i = 0; i < game->sets.len; i++) {
    Bag set = game->sets.dat[i];

    if (set.red > bag.red) {
      bag.red = set.red;
    }

    if (set.green > bag.green) {
      bag.green = set.green;
    }

    if (set.blue > bag.blue) {
      bag.blue = set.blue;
    }
  }

  return bag;
}

void solve(Bag bag, Span input) {
  u64 part1 = 0;
  u64 part2 = 0;
  SpanSplitIterator line_it = Span_split_lines(input);

  Game game = {0};

  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  while (line.valid) {
    Game_parse(&game, line.dat);

    if (Game_possible(&game, bag)) {
      part1 += game.id;
    }

    Bag minimum = Game_minimum(&game);
    u64 power = (u64)minimum.red * (u64)minimum.green * (u64)minimum.blue;
    part2 += power;

    memset(&game, 0, sizeof(Game));

    line = SpanSplitIterator_next(&line_it);
  }

  String out = {0};
  String_push_u64(&out, part1, 10);
  String_push_str(&out, " ");
  String_push_u64(&out, part2, 10);
  String_println(&out);
}

int main(void) {
  Bag bag = {
      .red = 12,
      .green = 13,
      .blue = 14,
  };

  Span example = Span_from_str(
      "Game 1: 3 blue, 4 red; 1 red, 2 green, 6 blue; 2 green\n"
      "Game 2: 1 blue, 2 green; 3 green, 4 blue, 1 red; 1 green, 1 blue\n"
      "Game 3: 8 green, 6 blue, 20 red; 5 blue, 4 red, 13 green; 5 green, 1 "
      "red\n"
      "Game 4: 1 green, 3 red, 6 blue; 3 green, 6 red; 3 green, 15 blue, 14 "
      "red\n"
      "Game 5: 6 red, 1 blue, 3 green; 2 blue, 1 red, 2 green\n");
  solve(bag, example);

  Span input = Span_from_file("inputs/day02.txt");
  solve(bag, input);

  return 0;
}
