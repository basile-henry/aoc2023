#include "baz.h"

typedef struct {
  u16 x;
  u16 y;
} Pos;

static inline bool Pos_eq(Pos a, Pos b) { return a.x == b.x && a.y == b.y; }

static inline usize Pos_to_ix(usize width, Pos p) {
  return (usize)p.x + (usize)p.y * (width + 1);
}

static inline Pos Pos_from_ix(usize width, usize i) {
  return (Pos){.x = (u16)(i % (width + 1)), .y = (u16)(i / (width + 1))};
}

define_array(PosArray, Pos, 4);

typedef struct {
  u64 dat[312]; // To accommodate 141 * 140 bits
} PosSet;

unused static bool PosSet_contains_pos(const PosSet *s, Pos p, usize width) {
  usize i = Pos_to_ix(width, p);
  return ((s->dat[(i >> 6)] >> (i & 0b111111)) & 1) == 1;
}

static void PosSet_insert_pos(PosSet *s, Pos p, usize width) {
  usize i = Pos_to_ix(width, p);
  assert((i >> 6) < 312);
  s->dat[(i >> 6)] |= ((u64)1 << (i & 0b111111));
}

static PosArray neighbours(Pos p, const u8 *maze, usize width, usize height) {
  usize i = Pos_to_ix(width, p);
  PosArray ns = {0};

  switch (maze[i]) {
  case '|':
    if (p.y > 0) {
      PosArray_push(&ns, (Pos){.x = p.x, .y = p.y - 1});
    }
    if (p.y < height - 1) {
      PosArray_push(&ns, (Pos){.x = p.x, .y = p.y + 1});
    }
    break;
  case '-':
    if (p.x > 0) {
      PosArray_push(&ns, (Pos){.x = p.x - 1, .y = p.y});
    }
    if (p.x < width - 1) {
      PosArray_push(&ns, (Pos){.x = p.x + 1, .y = p.y});
    }
    break;
  case 'L':
    if (p.y > 0) {
      PosArray_push(&ns, (Pos){.x = p.x, .y = p.y - 1});
    }
    if (p.x < width - 1) {
      PosArray_push(&ns, (Pos){.x = p.x + 1, .y = p.y});
    }
    break;
  case 'J':
    if (p.y > 0) {
      PosArray_push(&ns, (Pos){.x = p.x, .y = p.y - 1});
    }
    if (p.x > 0) {
      PosArray_push(&ns, (Pos){.x = p.x - 1, .y = p.y});
    }
    break;
  case '7':
    if (p.x > 0) {
      PosArray_push(&ns, (Pos){.x = p.x - 1, .y = p.y});
    }
    if (p.y < height - 1) {
      PosArray_push(&ns, (Pos){.x = p.x, .y = p.y + 1});
    }
    break;
  case 'F':
    if (p.x < width - 1) {
      PosArray_push(&ns, (Pos){.x = p.x + 1, .y = p.y});
    }
    if (p.y < height - 1) {
      PosArray_push(&ns, (Pos){.x = p.x, .y = p.y + 1});
    }
    break;
  case '.':
    break;
  case 'S':
    panic("Unknown neighbours\n");
    break;
  default:
    panic("Unexpected\n");
  }

  return ns;
}

static void solve(Span input) {
  usize width = UNWRAP(Span_split_on('\n', input)).fst.len;
  usize height = input.len / (width + 1);
  Pos start = Pos_from_ix(width, UNWRAP(Span_split_on('S', input)).fst.len);

  PosArray start_neighbours = {0};
  if (start.x > 0) {
    PosArray_push(&start_neighbours, (Pos){.x = start.x - 1, .y = start.y});
  }
  if (start.x < width - 1) {
    PosArray_push(&start_neighbours, (Pos){.x = start.x + 1, .y = start.y});
  }
  if (start.y > 0) {
    PosArray_push(&start_neighbours, (Pos){.x = start.x, .y = start.y - 1});
  }
  if (start.y < height - 1) {
    PosArray_push(&start_neighbours, (Pos){.x = start.x, .y = start.y + 1});
  }

  PosArray current = {0};
  PosArray next = {0};
  PosSet loop = {0};
  PosSet_insert_pos(&loop, start, width);

  u8 start_mask = 0;

  for (usize i = 0; i < start_neighbours.len; i++) {
    Pos n = start_neighbours.dat[i];
    PosArray ns = neighbours(n, input.dat, width, height);
    if (ns.len < 2) {
      continue;
    }
    if (Pos_eq(start, ns.dat[0])) {
      PosSet_insert_pos(&loop, n, width);
      PosArray_push(&current, n);
      PosArray_push(&next, ns.dat[1]);
    } else if (Pos_eq(start, ns.dat[1])) {
      PosSet_insert_pos(&loop, n, width);
      PosArray_push(&current, n);
      PosArray_push(&next, ns.dat[0]);
    } else {
      continue;
    }

    // UP
    if (n.y < start.y) {
      start_mask |= 0b0001;
    }
    // RIGHT
    if (n.x > start.x) {
      start_mask |= 0b0010;
    }
    // DOWN
    if (n.y > start.y) {
      start_mask |= 0b0100;
    }
    // LEFT
    if (n.x < start.x) {
      start_mask |= 0b1000;
    }
  }

  u8 s;

  switch (start_mask) {
  case 0b1010:
    s = '-';
    break;
  case 0b0101:
    s = '|';
    break;
  case 0b0110:
    s = 'F';
    break;
  case 0b0011:
    s = 'L';
    break;
  case 0b1100:
    s = '7';
    break;
  case 0b1001:
    s = 'J';
    break;
  default:
    panic("Unexpected\n");
  }

  assert(current.len == 2);
  assert(next.len == 2);

  usize part1 = 1;
  while (!Pos_eq(current.dat[0], current.dat[1])) {
    part1++;

    {
      PosArray ns = neighbours(next.dat[0], input.dat, width, height);

      if (Pos_eq(current.dat[0], ns.dat[0])) {
        current.dat[0] = next.dat[0];
        next.dat[0] = ns.dat[1];
      } else if (Pos_eq(current.dat[0], ns.dat[1])) {
        current.dat[0] = next.dat[0];
        next.dat[0] = ns.dat[0];
      } else {
        panic("Unexpected\n");
      }
      PosSet_insert_pos(&loop, current.dat[0], width);
    }

    {
      PosArray ns = neighbours(next.dat[1], input.dat, width, height);
      if (Pos_eq(current.dat[1], ns.dat[0])) {
        current.dat[1] = next.dat[1];
        next.dat[1] = ns.dat[1];
      } else if (Pos_eq(current.dat[1], ns.dat[1])) {
        current.dat[1] = next.dat[1];
        next.dat[1] = ns.dat[0];
      } else {
        panic("Unexpected\n");
      }
      PosSet_insert_pos(&loop, current.dat[1], width);
    }
  }

  usize part2 = 0;

  for (u16 y = 0; y < height; y++) {
    bool inside = false;
    u8 section_start = 0;

    for (u16 x = 0; x < width; x++) {
      Pos p = {.x = x, .y = y};
      if (PosSet_contains_pos(&loop, p, width)) {
        usize i = Pos_to_ix(width, p);

        u8 c = input.dat[i];

        if (c == 'S') {
          c = s;
        }

        switch (c) {
        case '-':
          break;
        case '|':
          inside = !inside;
          break;
        // Start section
        case 'F':
          section_start = 'F';
          break;
        case 'L':
          section_start = 'L';
          break;
        // End section
        case '7':
          if (section_start == 'L') {
            inside = !inside;
          }
          break;
        case 'J':
          if (section_start == 'F') {
            inside = !inside;
          }
          break;
        default:
          panic("Unexpected\n");
        }
      } else if (inside) {
        part2++;
      }
    }
  }

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example1 = Span_from_str("-L|F7\n"
                                "7S-7|\n"
                                "L|7||\n"
                                "-L-J|\n"
                                "L|-JF\n");
  solve(example1);

  Span example2 = Span_from_str("7-F7-\n"
                                ".FJ|7\n"
                                "SJLL7\n"
                                "|F--J\n"
                                "LJ.LJ\n");
  solve(example2);

  Span example3 = Span_from_str("...........\n"
                                ".S-------7.\n"
                                ".|F-----7|.\n"
                                ".||.....||.\n"
                                ".||.....||.\n"
                                ".|L-7.F-J|.\n"
                                ".|..|.|..|.\n"
                                ".L--J.L--J.\n"
                                "...........\n");
  solve(example3);

  Span example4 = Span_from_str(".F----7F7F7F7F-7....\n"
                                ".|F--7||||||||FJ....\n"
                                ".||.FJ||||||||L7....\n"
                                "FJL7L7LJLJ||LJ.L-7..\n"
                                "L--J.L7...LJS7F-7L7.\n"
                                "....F-J..F7FJ|L7L7L7\n"
                                "....L7.F7||L7|.L7L7|\n"
                                ".....|FJLJ|FJ|F7|.LJ\n"
                                "....FJL-7.||.||||...\n"
                                "....L---J.LJ.LJLJ...\n");
  solve(example4);

  Span example5 = Span_from_str("FF7FSF7F7F7F7F7F---7\n"
                                "L|LJ||||||||||||F--J\n"
                                "FL-7LJLJ||||||LJL-77\n"
                                "F--JF--7||LJLJ7F7FJ-\n"
                                "L---JF-JLJ.||-FJLJJ7\n"
                                "|F|F-JF---7F7-L7L|7|\n"
                                "|FFJF7L7F-JF7|JL---7\n"
                                "7-L-JL7||F7|L7F-7F7|\n"
                                "L.L7LFJ|||||FJL7||LJ\n"
                                "L7JLJL-JLJLJL--JLJ.L\n");
  solve(example5);

  Span input = Span_from_file("inputs/day10.txt");
  solve(input);

  return 0;
}
