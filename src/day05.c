#include "baz.h"

define_array(Seeds, u64, 32);

typedef struct {
  u64 dest;
  u64 source;
  u64 len;
} MapEntry;

unused static void MapEntry_print(MapEntry e) {
  putstr("{ .s = ");
  putu64(e.source);
  putstr(", .l = ");
  putu64(e.len);
  putstr(", .d = ");
  putu64(e.dest);
  putstr(" }");
}

static int MapEntry_cmp(const MapEntry *a, const MapEntry *b) {
  // Only sorting on source
  usize a_source = (usize)a->source;
  usize b_source = (usize)b->source;
  return usize_cmp(&a_source, &b_source);
}

define_array(Map, MapEntry, 64);

static MapEntry MapEntry_parse(Span input) {
  SpanParseU64 dest_res = Span_parse_u64(input, 10);
  assert(dest_res.valid);
  u64 dest = dest_res.dat.fst;
  assert(dest <= UINT32_MAX);

  SpanParseU64 source_res =
      Span_parse_u64(Span_slice(dest_res.dat.snd, 1, dest_res.dat.snd.len), 10);
  assert(source_res.valid);
  u64 source = source_res.dat.fst;
  assert(source <= UINT32_MAX);

  SpanParseU64 len_res = Span_parse_u64(
      Span_slice(source_res.dat.snd, 1, source_res.dat.snd.len), 10);
  assert(len_res.valid);
  u64 len = len_res.dat.fst;
  assert(len <= UINT32_MAX);

  return (MapEntry){
      .dest = (u64)dest,
      .source = (u64)source,
      .len = (u64)len,
  };
}

static void Map_parse(Map *map, Span *input) {
  SpanSplitIterator line_it = Span_split_lines(*input);
  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it); // empty line
  line = SpanSplitIterator_next(&line_it);                       // map name
  line = SpanSplitIterator_next(&line_it);

  while (line.valid && line.dat.len > 0) {
    MapEntry entry = MapEntry_parse(line.dat);
    usize ix = Map_bsearch(map, &entry, MapEntry_cmp);
    Map_insert(map, ix, entry);

    // Advance the input span
    *input = line_it.rest;
    line = SpanSplitIterator_next(&line_it);
  }
}

typedef struct {
  u64 start;
  u64 len;
} Range;

unused static void Range_print(Range r) {
  putstr("{ .s = ");
  putu64(r.start);
  putstr(", .l = ");
  putu64(r.len);
  putstr(" }");
}

#define MIN(a, b) (a < b ? a : b)

typedef Option(Range) OptRange;
static OptRange Range_interesect(Range a, Range b) {
  if (a.start > b.start) {
    swap(&a, &b, sizeof(Range));
  }

  // a.start is less than or equal to b.start
  u64 len = MIN(b.len, a.len - (b.start - a.start));
  if ((b.start < a.start + a.len) && (len > 0)) {
    // intersection
    return (OptRange){
        .dat =
            (Range){
                .start = b.start,
                .len = len,
            },
        .valid = true,
    };
  } else {
    return (OptRange){
        .valid = false,
    };
  }
}

define_array(Ranges, Range, 16);

static void Map_range(const Map *map, Ranges *out, Range in) {
  Ranges todo = {0};
  Ranges_push(&todo, in);

  while (todo.len > 0) {
    Range x = UNWRAP(Ranges_pop(&todo));

    for (usize i = 0; i < map->len; i++) {
      MapEntry entry = map->dat[i];
      OptRange intersection = Range_interesect(x, (Range){
                                                      .start = entry.source,
                                                      .len = entry.len,
                                                  });

      if (intersection.valid) {
        Range r = {
            .start = entry.dest + intersection.dat.start - entry.source,
            .len = intersection.dat.len,
        };
        Ranges_push(out, r);
        OptRange before = Range_interesect(x, (Range){
                                                  .start = 0,
                                                  .len = entry.source,
                                              });
        if (before.valid) {
          Ranges_push(&todo, before.dat);
        }

        OptRange after =
            Range_interesect(x, (Range){
                                    .start = entry.source + entry.len,
                                    .len = UINT32_MAX,
                                });
        if (after.valid) {
          Ranges_push(&todo, after.dat);
        }

        goto continue_outer;
      }
    }

    // No map matches
    Ranges_push(out, x);

  continue_outer:;
  }
}

static u64 Map_lookup(const Map *map, u64 key) {
  MapEntry entry = {
      .source = key,
  };

  usize ix = Map_bsearch(map, &entry, MapEntry_cmp);
  if (ix == map->len || map->dat[ix].source != key) {
    ix--;
  }

  MapEntry found = map->dat[ix];
  if (found.source <= key && key <= found.source + found.len) {
    return found.dest + (key - found.source);
  }

  return key;
}

static void solve(Span input) {
  Seeds seeds = {0};
  Map maps[7] = {0};

  // Parsing
  {
    SpanSplitOn split = Span_split_on('\n', input);
    assert(split.valid);

    Span seed_span = split.dat.fst;
    Span rest = split.dat.snd;

    SpanSplitIterator word_it = Span_split_words(seed_span);
    SpanSplitIteratorNext word = SpanSplitIterator_next(&word_it); // "seeds:"
    word = SpanSplitIterator_next(&word_it);

    while (word.valid) {
      u64 seed = UNWRAP(Span_parse_u64(word.dat, 10)).fst;
      assert(seed <= UINT32_MAX);

      Seeds_push(&seeds, (u64)seed);
      word = SpanSplitIterator_next(&word_it);
    }

    usize ix = 0;
    while (rest.len > 0) {
      Map_parse(&maps[ix], &rest);
      ix++;
    }
  }

  // Part 1
  u64 part1 = UINT32_MAX;
  {
    for (usize i = 0; i < seeds.len; i++) {
      u64 x = seeds.dat[i];

      for (usize j = 0; j < 7; j++) {
        x = Map_lookup(&maps[j], x);
      }

      part1 = x < part1 ? x : part1;
    }
  }

  // Part 2
  u64 part2 = UINT32_MAX;
  {
    Ranges a = {0};
    Ranges b = {0};

    Ranges *in = &a;
    Ranges *out = &b;

    for (usize i = 0; i < seeds.len; i += 2) {
      in->len = 0;
      Ranges_push(in, (Range){
                          .start = seeds.dat[i],
                          .len = seeds.dat[i + 1],
                      });

      for (usize j = 0; j < 7; j++) {
        out->len = 0;
        for (usize k = 0; k < in->len; k++) {
          Map_range(&maps[j], out, in->dat[k]);
        }
        swap(in, out, sizeof(Ranges));
      }

      for (usize k = 0; k < in->len; k++) {
        usize y = in->dat[k].start;
        part2 = y < part2 ? y : part2;
      }
    }
  }

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example = Span_from_str("seeds: 79 14 55 13\n"
                               "\n"
                               "seed-to-soil map:\n"
                               "50 98 2\n"
                               "52 50 48\n"
                               "\n"
                               "soil-to-fertilizer map:\n"
                               "0 15 37\n"
                               "37 52 2\n"
                               "39 0 15\n"
                               "\n"
                               "fertilizer-to-water map:\n"
                               "49 53 8\n"
                               "0 11 42\n"
                               "42 0 7\n"
                               "57 7 4\n"
                               "\n"
                               "water-to-light map:\n"
                               "88 18 7\n"
                               "18 25 70\n"
                               "\n"
                               "light-to-temperature map:\n"
                               "45 77 23\n"
                               "81 45 19\n"
                               "68 64 13\n"
                               "\n"
                               "temperature-to-humidity map:\n"
                               "0 69 1\n"
                               "1 0 69\n"
                               "\n"
                               "humidity-to-location map:\n"
                               "60 56 37\n"
                               "56 93 4\n");
  solve(example);

  Span input = Span_from_file("inputs/day05.txt");
  solve(input);

  return 0;
}
