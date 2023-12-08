#include "baz.h"

static u64 gcd(u64 a, u64 b) {
  if (b == 0)
    return a;
  return gcd(b, a % b);
}

static u64 lcm(u64 a, u64 b) { return ((a * b) / gcd(a, b)); }

typedef T2(u16, u16) Node;

define_array(Graph, Node, 1024);
define_hash_map(NodeName, Span, u16, 1024, Span_hash, Span_eq);

define_array(CountArray, usize, 8);
define_array(NodeIxArray, u16, 8);
define_bit_set(NodeIxSet, u64, 16);
_Static_assert(8 * sizeof(NodeIxSet) == 1024, "Unexpected NodeIxSet size");

static u16 NodeName_intern(NodeName *node_name, u16 *next_node_id, Span node) {
  u16 id = *NodeName_insert_modify(node_name, node, *next_node_id);

  if (id == *next_node_id) {
    (*next_node_id)++;
  }

  return id;
}

static void solve(Span input, bool do_part_1, bool do_part_2) {
  SpanSplitIterator line_it = Span_split_lines(input);

  Span instructions = UNWRAP(SpanSplitIterator_next(&line_it));
  UNWRAP(SpanSplitIterator_next(&line_it)); // Skip newline

  Graph graph = {0};
  u16 AAA_ix = 0;
  u16 ZZZ_ix = 0;

  NodeIxArray end_with_A = {0};
  NodeIxSet end_with_Z = {0};

  // Graph parsing
  {
    // Node name interning
    NodeName node_name = {0};
    u16 next_node_id = 0;

    // Mapping from node id to its index in the graph
    u16 node_id_to_ix[1024] = {0};

    SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
    while (line.valid) {
      Span name = Span_slice(line.dat, 0, 3);
      Span fst = Span_slice(line.dat, 7, 10);
      Span snd = Span_slice(line.dat, 12, 15);

      u16 node = NodeName_intern(&node_name, &next_node_id, name);
      u16 node_fst = NodeName_intern(&node_name, &next_node_id, fst);
      u16 node_snd = NodeName_intern(&node_name, &next_node_id, snd);

      u16 ix = (u16)graph.len;
      Graph_push(&graph, (Node){
                             .fst = node_fst,
                             .snd = node_snd,
                         });
      node_id_to_ix[(usize)node] = ix;

      if (Span_match(&name, "AAA")) {
        AAA_ix = ix;
      } else if (Span_match(&name, "ZZZ")) {
        ZZZ_ix = ix;
      }

      if (name.dat[2] == 'A') {
        NodeIxArray_push(&end_with_A, ix);
      }

      if (name.dat[2] == 'Z') {
        NodeIxSet_insert(&end_with_Z, ix);
      }

      line = SpanSplitIterator_next(&line_it);
    }

    // Fixup graph to use ix instead of IDs now that all IDs are resolved
    for (usize i = 0; i < graph.len; i++) {
      graph.dat[i].fst = node_id_to_ix[(usize)graph.dat[i].fst];
      graph.dat[i].snd = node_id_to_ix[(usize)graph.dat[i].snd];
    }
  }

  usize part1 = 0;
  if (do_part_1) {
    u16 ix = AAA_ix;
    bool found = false;

    while (!found) {
      for (usize i = 0; i < instructions.len; i++) {
        part1++;
        u8 dir = instructions.dat[i];

        switch (dir) {
        case 'L':
          ix = graph.dat[(usize)ix].fst;
          break;
        case 'R':
          ix = graph.dat[(usize)ix].snd;
          break;
        default:
          panic("Unexpected\n");
        }

        if (ix == ZZZ_ix) {
          found = true;
          break;
        }
      }
    }
  }

  usize part2 = 0;
  if (do_part_2) {
    CountArray first_time_at_end = {0};
    CountArray period = {0};
    NodeIxArray end_node_ix = {0};

    for (usize i = 0; i < end_with_A.len; i++) {
      usize count = 0;
      u16 ix = end_with_A.dat[i];
      bool found = false;

      while (!found) {
        for (usize j = 0; j < instructions.len; j++) {
          count++;
          u8 dir = instructions.dat[j];

          switch (dir) {
          case 'L':
            ix = graph.dat[(usize)ix].fst;
            break;
          case 'R':
            ix = graph.dat[(usize)ix].snd;
            break;
          default:
            panic("Unexpected\n");
          }

          if (NodeIxSet_contains(end_with_Z, ix)) {
            if (first_time_at_end.len == i) {
              // First time at end
              CountArray_push(&first_time_at_end, count);
              NodeIxArray_push(&end_node_ix, ix);
            } else if (end_node_ix.dat[i] == ix) {
              // Second time hitting the same node
              CountArray_push(&period, count - first_time_at_end.dat[i]);
              found = true;
              break;
            }
          }
        }
      }
    }

    part2 = 1;
    for (usize i = 0; i < first_time_at_end.len; i++) {
      // It looks like the input was nice and the offset and period are the same
      // Asserting it
      assert(first_time_at_end.dat[i] == period.dat[i]);

      part2 = lcm(part2, period.dat[i]);
    }
  }

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example1 = Span_from_str("RL\n"
                                "\n"
                                "AAA = (BBB, CCC)\n"
                                "BBB = (DDD, EEE)\n"
                                "CCC = (ZZZ, GGG)\n"
                                "DDD = (DDD, DDD)\n"
                                "EEE = (EEE, EEE)\n"
                                "GGG = (GGG, GGG)\n"
                                "ZZZ = (ZZZ, ZZZ)\n");
  solve(example1, true, false);

  Span example2 = Span_from_str("LLR\n"
                                "\n"
                                "AAA = (BBB, BBB)\n"
                                "BBB = (AAA, ZZZ)\n"
                                "ZZZ = (ZZZ, ZZZ)\n");
  solve(example2, true, false);

  Span example3 = Span_from_str("LR\n"
                                "\n"
                                "11A = (11B, XXX)\n"
                                "11B = (XXX, 11Z)\n"
                                "11Z = (11B, XXX)\n"
                                "22A = (22B, XXX)\n"
                                "22B = (22C, 22C)\n"
                                "22C = (22Z, 22Z)\n"
                                "22Z = (22B, 22B)\n"
                                "XXX = (XXX, XXX)\n");
  solve(example3, false, true);

  Span input = Span_from_file("inputs/day08.txt");
  solve(input, true, true);

  return 0;
}
