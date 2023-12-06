#include "baz.h"

define_array(TestArray, u8, 16);

static void test_array(void) {
  TestArray a = {0};

  TestArray_push(&a, 3);
  TestArray_push(&a, 4);
  TestArray_push(&a, 7);
  TestArray_push(&a, 42);
  TestArray_push(&a, 51);
  TestArray_push(&a, 127);

  {
    u8 entry = 5;
    usize ix = TestArray_bsearch(&a, &entry, u8_cmp);
    assert(ix == 2);
  }

  {
    u8 entry = 51;
    usize ix = TestArray_bsearch(&a, &entry, u8_cmp);
    assert(ix == 4);
  }

  assert(a.dat[2] == 7);
  assert(a.dat[5] == 127);
  TestArray_insert(&a, 2, 5);
  assert(a.dat[2] == 5);
  assert(a.dat[3] == 7);
  assert(a.dat[6] == 127);
}

int main(void) {
  test_array();
  printf0("Success\n");
  return 0;
}
