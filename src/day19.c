#include "baz.h"

typedef struct {
  u8 field;
  bool gt;
  u16 amount;
} Cond;

typedef struct {
  enum { GoToWorkflow, Accepted, Rejected } tag;
  Span workflow;
} Outcome;

typedef struct {
  bool with_condition;
  Cond cond;
  Outcome outcome;
} Rule;

static Cond Cond_parse(Span chunk) {
  assert(chunk.len > 2);
  return (Cond){
      .field = chunk.dat[0],
      .gt = (chunk.dat[1] == '>'),
      .amount =
          (u16)UNWRAP(Span_parse_u64(Span_slice(chunk, 2, chunk.len), 10)).fst,
  };
}

static Outcome Outcome_parse(Span chunk) {
  if (Span_match(&chunk, "A")) {
    return (Outcome){
        .tag = Accepted,
    };
  } else if (Span_match(&chunk, "R")) {
    return (Outcome){
        .tag = Rejected,
    };
  } else {
    return (Outcome){
        .tag = GoToWorkflow,
        .workflow = chunk,
    };
  }
}

static Rule Rule_parse(Span chunk) {
  SpanSplitOn res = Span_split_on(':', chunk);

  if (res.valid) {
    Cond cond = Cond_parse(res.dat.fst);
    return (Rule){
        .with_condition = true,
        .cond = cond,
        .outcome = Outcome_parse(res.dat.snd),
    };
  } else {
    return (Rule){
        .with_condition = false,
        .outcome = Outcome_parse(chunk),
    };
  }
}

define_array(Rules, Rule, 8);
define_hash_map(Workflows, Span, Rules, 1024, Span_hash, Span_eq);

typedef struct {
  u16 x;
  u16 m;
  u16 a;
  u16 s;
} Rating;

static bool Cond_match(Cond cond, Rating rating) {
  u16 field;

  switch (cond.field) {
  case 'x':
    field = rating.x;
    break;
  case 'm':
    field = rating.m;
    break;
  case 'a':
    field = rating.a;
    break;
  case 's':
    field = rating.s;
    break;
  default:
    panic("Unexpected!\n");
  }

  if (cond.gt) {
    return field > cond.amount;
  } else {
    return field < cond.amount;
  }
}

static Outcome Rules_outcome(const Rules *rules, Rating rating) {
  for (usize i = 0; i < rules->len; i++) {
    Rule rule = rules->dat[i];

    if (rule.with_condition) {
      if (Cond_match(rule.cond, rating)) {
        return rule.outcome;
      }
    } else {
      return rule.outcome;
    }
  }

  panic("Unexpected!\n");
}

static Rating Rating_parse(Span line) {
  assert(line.len > 1);

  SpanSplitIterator field_it = {
      .sep = ',',
      .rest = Span_slice(line, 1, line.len - 1),
  };

  Span x = UNWRAP(SpanSplitIterator_next(&field_it));
  Span m = UNWRAP(SpanSplitIterator_next(&field_it));
  Span a = UNWRAP(SpanSplitIterator_next(&field_it));
  Span s = UNWRAP(SpanSplitIterator_next(&field_it));

  return (Rating){
      .x = (u16)UNWRAP(Span_parse_u64(Span_slice(x, 2, x.len), 10)).fst,
      .m = (u16)UNWRAP(Span_parse_u64(Span_slice(m, 2, m.len), 10)).fst,
      .a = (u16)UNWRAP(Span_parse_u64(Span_slice(a, 2, a.len), 10)).fst,
      .s = (u16)UNWRAP(Span_parse_u64(Span_slice(s, 2, s.len), 10)).fst,
  };
}

static bool Rating_accepted(Workflows *workflows, Rating rating) {
  Span current = Span_from_str("in");

  while (true) {
    const Rules *rules = UNWRAP(Workflows_lookup(workflows, &current));
    Outcome outcome = Rules_outcome(rules, rating);

    switch (outcome.tag) {
    case GoToWorkflow:
      current = outcome.workflow;
      break;
    case Rejected:
      return false;
    case Accepted:
      return true;
    }
  }
}

static Rules Rules_parse(Span line) {
  assert(line.dat[line.len - 1] == '}');
  SpanSplitIterator chunk_it = {
      .sep = ',',
      .rest = Span_slice(line, 0, line.len - 1),
  };

  Rules rules = {0};

  SpanSplitIteratorNext chunk = SpanSplitIterator_next(&chunk_it);
  while (chunk.valid) {
    Rules_push(&rules, Rule_parse(chunk.dat));
    chunk = SpanSplitIterator_next(&chunk_it);
  }

  return rules;
}

typedef struct {
  Span current;
  Rating min;
  Rating max;
} SearchState;

define_array(Search, SearchState, 256);

#define MAX(A, B) (A > B ? A : B)
#define MIN(A, B) (A < B ? A : B)

static usize Workflows_count_distinct(Workflows *workflows) {
  usize count = 0;
  Search search = {0};
  Search_push(&search, (SearchState){
                           .current = Span_from_str("in"),
                           .min = {.x = 1, .m = 1, .a = 1, .s = 1},
                           .max = {.x = 4000, .m = 4000, .a = 4000, .s = 4000},
                       });

  SearchPop next = Search_pop(&search);
  while (next.valid) {
    const Rules *rules = UNWRAP(Workflows_lookup(workflows, &next.dat.current));

    for (usize i = 0; i < rules->len; i++) {
      Rule rule = rules->dat[i];

      SearchState state = next.dat;
      if (rule.with_condition) {
        switch (rule.cond.field) {
        case 'x':
          if (rule.cond.gt) {
            state.min.x = (u16)MAX(state.min.x, rule.cond.amount + 1);
            next.dat.max.x = (u16)MIN(next.dat.max.x, rule.cond.amount);
          } else {
            state.max.x = (u16)MIN(state.max.x, rule.cond.amount - 1);
            next.dat.min.x = (u16)MAX(next.dat.min.x, rule.cond.amount);
          }
          break;
        case 'm':
          if (rule.cond.gt) {
            state.min.m = (u16)MAX(state.min.m, rule.cond.amount + 1);
            next.dat.max.m = (u16)MIN(next.dat.max.m, rule.cond.amount);
          } else {
            state.max.m = (u16)MIN(state.max.m, rule.cond.amount - 1);
            next.dat.min.m = (u16)MAX(next.dat.min.m, rule.cond.amount);
          }
          break;
        case 'a':
          if (rule.cond.gt) {
            state.min.a = (u16)MAX(state.min.a, rule.cond.amount + 1);
            next.dat.max.a = (u16)MIN(next.dat.max.a, rule.cond.amount);
          } else {
            state.max.a = (u16)MIN(state.max.a, rule.cond.amount - 1);
            next.dat.min.a = (u16)MAX(next.dat.min.a, rule.cond.amount);
          }
          break;
        case 's':
          if (rule.cond.gt) {
            state.min.s = (u16)MAX(state.min.s, rule.cond.amount + 1);
            next.dat.max.s = (u16)MIN(next.dat.max.s, rule.cond.amount);
          } else {
            state.max.s = (u16)MIN(state.max.s, rule.cond.amount - 1);
            next.dat.min.s = (u16)MAX(next.dat.min.s, rule.cond.amount);
          }
          break;
        default:
          panic("Unexpected\n");
        }
      }

      switch (rule.outcome.tag) {
      case GoToWorkflow:
        state.current = rule.outcome.workflow;
        Search_push(&search, state);
        break;
      case Rejected:
        break;
      case Accepted: {
        usize dx =
            (usize)(state.max.x >= state.min.x ? 1 + state.max.x - state.min.x
                                               : 0);
        usize dm =
            (usize)(state.max.m >= state.min.m ? 1 + state.max.m - state.min.m
                                               : 0);
        usize da =
            (usize)(state.max.a >= state.min.a ? 1 + state.max.a - state.min.a
                                               : 0);
        usize ds =
            (usize)(state.max.s >= state.min.s ? 1 + state.max.s - state.min.s
                                               : 0);

        count += dx * dm * da * ds;
        break;
      }
      }
    }

    next = Search_pop(&search);
  }

  return count;
}

static void solve(Span input) {
  SpanSplitIterator line_it = Span_split_lines(input);

  Workflows workflows = {0};
  bool reading_ratings = false;
  usize part1 = 0;

  SpanSplitIteratorNext line = SpanSplitIterator_next(&line_it);
  while (line.valid) {
    if (line.dat.len == 0) {
      reading_ratings = true;
      line = SpanSplitIterator_next(&line_it);
      continue;
    }

    if (reading_ratings) {
      Rating rating = Rating_parse(line.dat);

      if (Rating_accepted(&workflows, rating)) {
        part1 += (usize)rating.x;
        part1 += (usize)rating.m;
        part1 += (usize)rating.a;
        part1 += (usize)rating.s;
      }
    } else {
      SpanSplitOn res = Span_split_on('{', line.dat);
      assert(res.valid);

      Workflows_insert(&workflows, res.dat.fst, Rules_parse(res.dat.snd));
    }

    line = SpanSplitIterator_next(&line_it);
  }

  usize part2 = Workflows_count_distinct(&workflows);

  printf2("%u | %u\n", part1, part2);
}

int main(void) {
  Span example = Span_from_str("px{a<2006:qkq,m>2090:A,rfg}\n"
                               "pv{a>1716:R,A}\n"
                               "lnx{m>1548:A,A}\n"
                               "rfg{s<537:gd,x>2440:R,A}\n"
                               "qs{s>3448:A,lnx}\n"
                               "qkq{x<1416:A,crn}\n"
                               "crn{x>2662:A,R}\n"
                               "in{s<1351:px,qqz}\n"
                               "qqz{s>2770:qs,m<1801:hdj,R}\n"
                               "gd{a>3333:R,R}\n"
                               "hdj{m>838:A,pv}\n"
                               "\n"
                               "{x=787,m=2655,a=1222,s=2876}\n"
                               "{x=1679,m=44,a=2067,s=496}\n"
                               "{x=2036,m=264,a=79,s=2244}\n"
                               "{x=2461,m=1339,a=466,s=291}\n"
                               "{x=2127,m=1623,a=2188,s=1013}\n");
  solve(example);

  Span input = Span_from_file("inputs/day19.txt");
  solve(input);

  return 0;
}
