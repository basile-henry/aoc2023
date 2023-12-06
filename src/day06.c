#include "baz.h"

typedef struct {
  u64 time;
  u64 distance;
} Race;

define_array(Races, Race, 4);

//  - time = charge-time + run-time
//  - distance = run-time * speed
//  - speed = charge-time * charge-amount
//
// charge-amount = 1mm/ms
// Which gives:
//
// distance = run-time * charge-time
static u64 ways(Race race) {
  // ways = max-charge-time - min-charge-time
  // symetric: mid-time - min-charge-time == mid-time - max-charge-time
  // ==> ways == 2 * (mid-time - min-charge-time)
  // Note: watch out for off by one for even/odd times

  // binary search for the first charge time that beats the distance
  u64 mid = race.time >> 1;

  u64 min_charge_time = 0;
  u64 dc = mid;

  do {
    dc >>= 1;

    u64 charge_time = min_charge_time + dc;
    u64 run_time = race.time - charge_time;

    if (run_time * charge_time > race.distance) {
      /* left */
    } else {
      /* right */
      min_charge_time += dc + 1;
    }
  } while (dc > 0);

  if ((race.time & 1) == 0) {
    return 2 * (mid - min_charge_time) + 1;
  } else {
    return 2 * (1 + mid - min_charge_time);
  }
}

static void solve(const Races *races) {
  u64 res = 1;

  for (usize i = 0; i < races->len; i++) {
    u64 w = ways(races->dat[i]);
    res *= w;
  }

  putu64(res);
  putchar('\n');
}

int main(void) {
  {
    Races example = {0};
    Races_push(&example, (Race){
                             .time = 7,
                             .distance = 9,
                         });
    Races_push(&example, (Race){
                             .time = 15,
                             .distance = 40,
                         });
    Races_push(&example, (Race){
                             .time = 30,
                             .distance = 200,
                         });
    solve(&example);
  }
  {
    Races example = {0};
    Races_push(&example, (Race){
                             .time = 71530,
                             .distance = 940200,
                         });
    solve(&example);
  }

  {
    Races input = {0};
    Races_push(&input, (Race){
                           .time = 34,
                           .distance = 204,
                       });
    Races_push(&input, (Race){
                           .time = 90,
                           .distance = 1713,
                       });
    Races_push(&input, (Race){
                           .time = 89,
                           .distance = 1210,
                       });
    Races_push(&input, (Race){
                           .time = 86,
                           .distance = 1780,
                       });
    solve(&input);
  }
  {
    Races input = {0};
    Races_push(&input, (Race){
                           .time = 34908986,
                           .distance = 204171312101780,
                       });
    solve(&input);
  }

  return 0;
}
