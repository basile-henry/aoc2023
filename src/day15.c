#include "baz.h"

static u8 hash(Span input) {
  u8 x = 0;

  for (usize i = 0; i < input.len; i++) {
    // ignore newline characters
    if (input.dat[i] == '\n') {
      continue;
    }

    x += input.dat[i];
    x *= 17;
  }

  return x;
}

typedef struct {
  Span label;
  u8 value;
} Entry;

define_array(Box, Entry, 8);

typedef struct {
  Box boxes[256];
} HASHMAP;

static void HASHMAP_insert(HASHMAP *hm, Entry e) {
  u8 ix = hash(e.label);
  Box *box = &hm->boxes[ix];

  for (usize i = 0; i < box->len; i++) {
    if (Span_eq(&box->dat[i].label, &e.label)) {
      box->dat[i].value = e.value;
      return;
    }
  }

  Box_push(box, e);
}

static void HASHMAP_remove(HASHMAP *hm, Span label) {
  u8 ix = hash(label);
  Box *box = &hm->boxes[ix];

  for (usize i = 0; i < box->len; i++) {
    if (Span_eq(&box->dat[i].label, &label)) {
      Box_remove(box, i);
      return;
    }
  }
}

static usize HASHMAP_focusing_power(const HASHMAP *hm) {
  usize power = 0;
  for (usize i = 0; i < 256; i++) {
    for (usize j = 0; j < hm->boxes[i].len; j++) {
      power += (1 + i) * (1 + j) * hm->boxes[i].dat[j].value;
    }
  }

  return power;
}

static void solve(Span input) {
  SpanSplitIterator chunk_it = {
      .rest = input,
      .sep = ',',
  };

  usize part1 = 0;
  HASHMAP hm = {0};

  SpanSplitIteratorNext chunk = SpanSplitIterator_next(&chunk_it);
  while (chunk.valid) {
    part1 += (usize)hash(chunk.dat);

    SpanSplitOn remove = Span_split_on('-', chunk.dat);
    SpanSplitOn insert = Span_split_on('=', chunk.dat);

    if (remove.valid) {
      HASHMAP_remove(&hm, remove.dat.fst);
    } else if (insert.valid) {
      Entry e = {
          .label = insert.dat.fst,
          .value = (u8)UNWRAP(Span_parse_u64(insert.dat.snd, 10)).fst,
      };
      HASHMAP_insert(&hm, e);
    } else {
      panic("Unexpected\n");
    }

    chunk = SpanSplitIterator_next(&chunk_it);
  }

  usize part2 = HASHMAP_focusing_power(&hm);

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  printf1("hash(\"HASH\"): %u\n", hash(Span_from_str("HASH")));

  Span example =
      Span_from_str("rn=1,cm-,qp=3,cm=2,qp-,pc=4,ot=9,ab=5,pc-,pc=6,ot=7");
  solve(example);

  Span input = Span_from_file("inputs/day15.txt");
  solve(input);

  return 0;
}
