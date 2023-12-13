#include "baz.h"

define_array(Groups, u8, 32);

typedef struct {
  Span locations;
  Groups groups;
} Springs;

static Springs Springs_parse(Span line) {
  SpanSplitOn res = Span_split_on(' ', line);
  assert(res.valid);

  Groups groups = {0};
  SpanSplitIterator group_it = {
      .rest = res.dat.snd,
      .sep = ',',
  };

  SpanSplitIteratorNext group = SpanSplitIterator_next(&group_it);
  while (group.valid) {
    Groups_push(&groups, (u8)UNWRAP(Span_parse_u64(group.dat, 10)).fst);
    group = SpanSplitIterator_next(&group_it);
  }

  return (Springs){
      .locations = res.dat.fst,
      .groups = groups,
  };
}

#define CACHE_SIZE (32 * 256)

typedef struct {
  u64 dat[CACHE_SIZE];
} Cache;

static usize Springs_arrangements_rec(Cache *cache, Springs springs,
                                      u8 loc_offset, u8 group_offset,
                                      u8 groups_space) {
  u64 key = (u64)loc_offset | (u64)((u64)group_offset << 8);
  assert(key < CACHE_SIZE);
  if (cache->dat[key] != 0) {
    return cache->dat[key];
  }

  if (group_offset == springs.groups.len) {
    for (usize i = loc_offset; i < springs.locations.len; i++) {
      if (springs.locations.dat[i] == '#') {
        return 0;
      }
    }
    return 1;
  }

  if ((loc_offset + groups_space) > springs.locations.len) {
    return 0;
  }

  usize arrangements = 0;
  u8 group = springs.groups.dat[group_offset];

  if (loc_offset + group == springs.locations.len ||
      springs.locations.dat[loc_offset + group] != '#') {
    bool valid_placement = true;
    for (usize i = loc_offset; i < loc_offset + group; i++) {
      if (springs.locations.dat[i] == '.') {
        valid_placement = false;
        break;
      }
    }

    if (valid_placement) {
      arrangements += Springs_arrangements_rec(
          cache, springs, (u8)(loc_offset + group + 1), group_offset + 1,
          (u8)(groups_space - group - 1));
    }
  }

  if (springs.locations.dat[loc_offset] != '#') {
    // We don't have to place it here, so we can try with the next valid
    // offset
    u8 next_loc = loc_offset + 1;
    u8 count = 0;
    u8 saw_required = false;
    while (next_loc + groups_space < springs.locations.len && count < group) {
      u8 c = springs.locations.dat[(usize)(next_loc + count)];

      if (c == '.') {
        next_loc += (u8)(count + 1);
        count = 0;

        if (saw_required) {
          cache->dat[key] = arrangements;
          return arrangements;
        }
      } else {
        saw_required |= (c == '#');
        count++;
      }
    }

    if ((next_loc + groups_space) <= springs.locations.len) {
      arrangements += Springs_arrangements_rec(cache, springs, next_loc,
                                               group_offset, groups_space);
    }
  }

  cache->dat[key] = arrangements;
  return arrangements;
}

static usize Springs_arrangements(Springs springs) {
  Cache cache = {0};

  u8 groups_space = (u8)springs.groups.len - 1; // In between space
  for (usize i = 0; i < springs.groups.len; i++) {
    groups_space += springs.groups.dat[i];
  }

  usize n = Springs_arrangements_rec(&cache, springs, 0, 0, groups_space);

  return n;
}

static void solve(Span input) {
  SpanSplitIterator line_it = Span_split_lines(input);

  usize part1 = 0;
  usize part2 = 0;

  String locations = {0};
  Groups groups = {0};

  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  while (line.valid) {
    Springs springs = Springs_parse(line.dat);

    String_clear(&locations);
    groups.len = 0;
    for (usize i = 0; i < 5; i++) {
      if (i > 0) {
        String_push(&locations, '?');
      }
      String_push_span(&locations, springs.locations);
      for (usize j = 0; j < springs.groups.len; j++) {
        Groups_push(&groups, springs.groups.dat[j]);
      }
    }

    part1 += Springs_arrangements(springs);
    part2 += Springs_arrangements((Springs){
        .locations =
            {
                .dat = locations.dat,
                .len = locations.len,
            },
        .groups = groups,
    });

    line = SpanSplitIterator_next(&line_it);
  }

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example = Span_from_str("???.### 1,1,3\n"
                               ".??..??...?##. 1,1,3\n"
                               "?#?#?#?#?#?#?#? 1,3,1,6\n"
                               "????.#...#... 4,1,1\n"
                               "????.######..#####. 1,6,5\n"
                               "?###???????? 3,2,1\n");
  solve(example);

  Span input = Span_from_file("inputs/day12.txt");
  solve(input);

  return 0;
}
