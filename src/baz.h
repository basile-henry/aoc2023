#ifndef BAZ_HEADER
#define BAZ_HEADER

#define unused __attribute__((unused))
#define private unused static

#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define panic(msg)                                                             \
  do {                                                                         \
    print_msg_with_loc(__FILE__, __LINE__, msg, strlen(msg));                  \
    sys_exit(1);                                                               \
    __builtin_unreachable();                                                   \
  } while (0)

#define assert(cond)                                                           \
  do {                                                                         \
    if (unlikely(!(cond))) {                                                   \
      const char *msg = "Assertion failed";                                    \
      print_msg_with_loc(__FILE__, __LINE__, msg, strlen(msg));                \
      sys_exit(1);                                                             \
    }                                                                          \
  } while (0)

#define assert_msg(cond, msg)                                                  \
  do {                                                                         \
    if (unlikely(!(cond))) {                                                   \
      print_msg_with_loc(__FILE__, __LINE__, msg, strlen(msg));                \
      sys_exit(1);                                                             \
    }                                                                          \
  } while (0)

#define Span_debug(span)                                                       \
  print_msg_with_loc(__FILE__, __LINE__, (const char *)span.dat, span.len)

///////////////////////////////////////////////////////////////////////////////
// Types

#define NULL 0

// C++ already has bool defined (C++ not supported, but this keeps clangd happy)
#ifndef __cplusplus
typedef _Bool bool;
#define true 1
#define false 0
#endif

typedef unsigned char u8;
_Static_assert(sizeof(u8) == 1, "u8 should be 1 byte");
typedef unsigned short u16;
_Static_assert(sizeof(u16) == 2, "u16 should be 2 byte");
typedef unsigned int u32;
_Static_assert(sizeof(u32) == 4, "u32 should be 4 byte");
typedef unsigned long u64;
_Static_assert(sizeof(u64) == 8, "u64 should be 8 byte");

typedef char i8;
_Static_assert(sizeof(i8) == 1, "i8 should be 1 byte");
typedef short i16;
_Static_assert(sizeof(i16) == 2, "i16 should be 2 byte");
typedef int i32;
_Static_assert(sizeof(i32) == 4, "i32 should be 4 byte");
typedef long i64;
_Static_assert(sizeof(i64) == 8, "i64 should be 8 byte");

typedef i64 isize;
typedef u64 usize;

#define UINT8_MAX 255
#define UINT16_MAX 65535
#define UINT32_MAX 4294967295
#define UINT64_MAX 18446744073709551615L

///////////////////////////////////////////////////////////////////////////////
// Syscalls

// From glibc
#define STDOUT 1
#define STDERR 2
#define O_RDONLY 0
#define SEEK_END 2
#define PROT_READ 0x1
#define PROT_WRITE 0x2
#define MAP_PRIVATE 0x02
#define MAP_ANONYMOUS 0x20

isize sys_write(i32 fd, const void *buf, usize size) {
  register i64 rax __asm__("rax") = 1;
  register i32 rdi __asm__("rdi") = fd;
  register const void *rsi __asm__("rsi") = buf;
  register usize rdx __asm__("rdx") = size;
  __asm__ __volatile__("syscall"
                       : "+r"(rax)
                       : "r"(rdi), "r"(rsi), "r"(rdx)
                       : "rcx", "r11", "memory");
  return rax;
}

isize sys_lseek(i32 fd, isize offset, usize origin) {
  register i64 rax __asm__("rax") = 8;
  register i32 rdi __asm__("rdi") = fd;
  register isize rsi __asm__("rsi") = offset;
  register usize rdx __asm__("rdx") = origin;
  __asm__ __volatile__("syscall"
                       : "+r"(rax)
                       : "r"(rdi), "r"(rsi), "r"(rdx)
                       : "rcx", "r11", "memory");
  return rax;
}

i32 sys_open(const char *filename, i32 flags, i32 mode) {
  register i64 rax __asm__("rax") = 2;
  register const char *rdi __asm__("rdi") = filename;
  register i32 rsi __asm__("rsi") = flags;
  register i32 rdx __asm__("rdx") = mode;
  __asm__ __volatile__("syscall"
                       : "+r"(rax)
                       : "r"(rdi), "r"(rsi), "r"(rdx)
                       : "rcx", "r11", "memory");
  return (i32)rax;
}

void *sys_mmap(void *addr, usize length, i32 prot, i32 flags, i32 fd,
               isize offset) {
  register i64 rax __asm__("rax") = 9;
  register usize rdi __asm__("rdi") = (usize)addr;
  register usize rsi __asm__("rsi") = length;
  register usize rdx __asm__("rdx") = (usize)prot;
  register usize r10 __asm__("r10") = (usize)flags;
  register usize r8 __asm__("r8") = (usize)fd;
  register usize r9 __asm__("r9") = (usize)offset;
  __asm__ __volatile__("syscall"
                       : "+r"(rax)
                       : "r"(rdi), "r"(rsi), "r"(rdx), "r"(r10), "r"(r8),
                         "r"(r9)
                       : "rcx", "r11", "memory");
  return (void *)rax;
}

void sys_exit(i32 exit_status) {
  register i64 rax __asm__("rax") = 60;
  register i32 rdi __asm__("rdi") = exit_status;
  __asm__ __volatile__("syscall"
                       : "+r"(rax)
                       : "r"(rdi)
                       : "rcx", "r11", "memory");
  __builtin_unreachable();
}

///////////////////////////////////////////////////////////////////////////////
// Entry point

int main(void);

static usize _start_argc;
static char const *const *_start_argv;

__attribute__((force_align_arg_pointer)) void _start() {
  // Not sure why the extra offsets, it looks like both GCC and Clang push a
  // couple values to the stack before getting here. I hope it's reliable
  __asm__ __volatile__("mov 16(%%rsp), %0" : "=r"(_start_argc)::);
  __asm__ __volatile__("lea 24(%%rsp), %0" : "=r"(_start_argv)::);

  int ret = main();
  sys_exit(ret);
}

///////////////////////////////////////////////////////////////////////////////
// Mem utils

// Needed by C compiler for copying data
extern void *memcpy(void *__restrict dst, const void *__restrict src,
                    usize bytes) {
  u8 *dst_bytes = (u8 *)dst;
  const u8 *src_bytes = (const u8 *)src;

  // naive impl
  for (usize i = 0; i < bytes; i++) {
    dst_bytes[i] = src_bytes[i];
  }

  return dst;
}

// Needed by C compiler for zero-initialisation
extern void *memset(void *s, int c, usize bytes) {
  u8 *s_byte = (u8 *)s;

  // naive impl
  for (usize i = 0; i < bytes; i++) {
    s_byte[i] = (u8)c;
  }

  return s;
}

private
void *calloc(usize n_elem, usize size_elem) {
  return sys_mmap(NULL, n_elem * size_elem, PROT_READ | PROT_WRITE,
                  MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
}

private
void free(void *x) {
  // TODO: call munmap?
  (void)(x);
}

private
void swap(void *__restrict a, void *__restrict b, usize bytes) {
  u8 temp[bytes]; // VLA
  memcpy(temp, a, bytes);
  memcpy(a, b, bytes);
  memcpy(b, temp, bytes);
}

private
int memcmp(const void *a, const void *b, usize bytes) {
  const u8 *a_bytes = (const u8 *)a;
  const u8 *b_bytes = (const u8 *)b;

  // naive impl
  for (usize i = 0; i < bytes; i++) {
    if (a_bytes[i] != b_bytes[i]) {
      return (a_bytes[i] < b_bytes[i]) ? -1 : 1;
    }
  }

  return 0;
}

private
void *memchr(const void *s, int c, usize bytes) {
  const u8 *s8 = (const u8 *)s;
  u8 c8 = (u8)c;

  // naive impl
  for (usize i = 0; i < bytes; i++) {
    if (s8[i] == c8) {
      return (void *)((usize)s + i);
    }
  }

  return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// C-style 0-terminated string utils

private
usize strlen(const char *str) {
  const char *ptr = str;
  while (*ptr != '\0') {
    ptr++;
  }

  return (usize)(ptr - str);
}

// We implement this later with a better error message when we can format
// __LINE__ properly
static void print_msg_with_loc(const char *file, u64 line, const char *msg,
                               usize msg_len);

///////////////////////////////////////////////////////////////////////////////
// Printing/Parsing

private
inline u8 to_digit(u64 x, u8 base) {
  assert(base <= 36);
  assert(x < base);

  if (x < 10) {
    return (u8)x + '0';
  } else {
    return (u8)x - 10 + 'A';
  }
}

private
inline bool is_digit(u8 x, u8 base) {
  assert(base <= 36);
  if (base <= 10) {
    return (x >= '0' && x < (base + '0'));
  } else {
    return (x >= '0' && x <= '9') || (x >= 'a' && x < (base - 10 + 'a')) ||
           (x >= 'A' && x < (base - 10 + 'A'));
  }
}

private
inline u64 from_digit(u8 x, u8 base) {
  assert(is_digit(x, base));

  if (x <= '9') {
    return (u64)(x - '0');
  } else if (x <= 'Z') {
    return (u64)(10 + x - 'A');
  } else {
    return (u64)(10 + x - 'a');
  }
}

// format number x with base into buffer
private
usize fmt_u64(u8 *buf, usize buf_len, u64 x, u8 base) {
  assert(buf_len > 0);
  if (x == 0) {
    *buf = '0';
    return 1;
  }

  // write in reverse (we don't know how many bytes it will be)
  usize i = 0;
  while (x > 0) {
    assert(i < buf_len); // crash otherwise

    buf[i] = to_digit(x % base, base);
    x /= base;
    i++;
  }

  // reverse
  for (usize j = 0; j < i / 2; j++) {
    u8 t = buf[j];
    buf[j] = buf[i - 1 - j];
    buf[i - 1 - j] = t;
  }

  return i;
}

private
usize fmt_i64(u8 *buf, usize buf_len, i64 x, u8 base) {
  if (x < 0) {
    assert(buf_len > 1);
    buf[0] = '-';
    usize len = fmt_u64(&buf[1], buf_len - 1, (u64)(-x), base);
    return len + 1;
  } else {
    return fmt_u64(buf, buf_len, (u64)x, base);
  }
}

// Changes buf_len to the parse len
private
u64 parse_u64(const u8 *buf, usize *buf_len, u8 base) {
  usize i = 0;
  u64 x = 0;
  while (i < *buf_len && is_digit(buf[i], base)) {
    x *= (u64)base;
    x += from_digit(buf[i], base);
    i++;
  }

  *buf_len = i;
  return x;
}

// Changes buf_len to the parse len
private
i64 parse_i64(const u8 *buf, usize *buf_len, u8 base) {
  if (*buf_len > 0 && buf[0] == '-') {
    usize len = *buf_len - 1;
    u64 x = parse_u64(&buf[1], &len, base);
    *buf_len = (len == 0) ? 0 : len + 1; // handle parse error
    return -(i64)x;
  } else {
    u64 x = parse_u64(buf, buf_len, base);
    return (i64)x;
  }
}

///////////////////////////////////////////////////////////////////////////////
// Basic IO

private
int putchar(int c) {
  sys_write(STDOUT, (u8 *)&c, 1);
  return c;
}

private
inline void putstr(const char *s) { sys_write(STDOUT, s, strlen(s)); }

private
void putu64(u64 x) {
  u8 buf[128];
  usize len = fmt_u64(buf, 128, x, 10);
  sys_write(STDOUT, buf, len);
}

private
void puti64(i64 x) {
  u8 buf[128];
  usize len = fmt_i64(buf, 128, x, 10);
  sys_write(STDOUT, buf, len);
}

///////////////////////////////////////////////////////////////////////////////
// Int utils

private
inline u8 u64_to_u8(u64 x) {
  assert(x <= UINT8_MAX);

  return (u8)x;
}

private
inline u16 u64_to_u16(u64 x) {
  assert(x <= UINT16_MAX);

  return (u16)x;
}

#define ABS_DIFF(T, X, Y)                                                      \
  ({                                                                           \
    T x = X;                                                                   \
    T y = Y;                                                                   \
    x > y ? x - y : y - x;                                                     \
  })

///////////////////////////////////////////////////////////////////////////////
// Hash

// FxHash using this reference:
// https://github.com/rust-lang/rustc-hash/blob/5e09ea0a1c7ab7e4f9e27771f5a0e5a36c58d1bb/src/lib.rs#L79
typedef usize FxHasher;
typedef usize Hash;

#define K 0x517cc1b727220a95u

private
inline void FxHasher_add(FxHasher *hasher, usize x) {
  *hasher = (*hasher << 5) | (*hasher >> (8 * sizeof(usize) - 5));
  *hasher ^= x;
  *hasher *= K;
}

private
Hash usize_hash(const usize *x) {
  FxHasher hasher = {0};
  FxHasher_add(&hasher, *x);
  return hasher;
}

private
bool usize_eq(const usize *a, const usize *b) { return *a == *b; }

private
int usize_cmp(const usize *a, const usize *b) {
  return *a < *b ? -1 : *a == *b ? 0 : 1;
}

private
bool u8_eq(const u8 *a, const u8 *b) { return *a == *b; }

private
int u8_cmp(const u8 *a, const u8 *b) { return *a < *b ? -1 : *a == *b ? 0 : 1; }

////////////////////////////////////////////////////////////////////////////////
// Tuples

#define T2(X, Y)                                                               \
  struct {                                                                     \
    X fst;                                                                     \
    Y snd;                                                                     \
  }

////////////////////////////////////////////////////////////////////////////////
// Optional

#define Option(T)                                                              \
  struct {                                                                     \
    T dat;                                                                     \
    bool valid;                                                                \
  }

#define UNWRAP(E)                                                              \
  ({                                                                           \
    __auto_type UNWRAP_res = E;                                                \
    assert(UNWRAP_res.valid);                                                  \
    UNWRAP_res.dat;                                                            \
  })

////////////////////////////////////////////////////////////////////////////////
// Array

/*
Define a capacity-bounded, but dynamically growing array with element type T
and capacity N.

For example `define_array(WordsArray, Span, 32);` defines the new type
`WordsArray` along with various methods such as `WordsArray_push`,
`WordsArray_pop`, ...
*/
#define define_array(A_NAME, T, N)                                             \
  typedef struct {                                                             \
    usize len;                                                                 \
    T dat[N];                                                                  \
  } A_NAME;                                                                    \
                                                                               \
  const usize A_NAME##_capacity = N;                                           \
                                                                               \
private                                                                        \
  T *A_NAME##_push(A_NAME *array, T x) {                                       \
    assert_msg(array->len < N, "push: max capacity");                          \
    T *slot = &array->dat[array->len];                                         \
    *slot = x;                                                                 \
    array->len += 1;                                                           \
    return slot;                                                               \
  }                                                                            \
                                                                               \
private                                                                        \
  T *A_NAME##_insert(A_NAME *array, usize ix, T x) {                           \
    assert_msg(array->len < N, "insert: max_capacity");                        \
    assert_msg(ix <= array->len, "insert: ix out of bound");                   \
                                                                               \
    array->len += 1;                                                           \
    for (usize i = ix; i < array->len; i++) {                                  \
      T t = array->dat[i];                                                     \
      array->dat[i] = x;                                                       \
      x = t;                                                                   \
    }                                                                          \
    return &array->dat[ix];                                                    \
  }                                                                            \
                                                                               \
  /* Assumes a sorted array */                                                 \
private                                                                        \
  usize A_NAME##_bsearch(const A_NAME *array, const T *entry,                  \
                         int cmp(const T *a, const T *b)) {                    \
    usize len = array->len;                                                    \
    usize ix = 0;                                                              \
                                                                               \
    while (len > 0) {                                                          \
      len = len >> 1;                                                          \
      usize mid = ix + len;                                                    \
      if (mid >= array->len) {                                                 \
        /* left */                                                             \
        continue;                                                              \
      }                                                                        \
                                                                               \
      int c = cmp(entry, &array->dat[mid]);                                    \
      if (c == 0) {                                                            \
        return mid;                                                            \
      } else if (c > 0) {                                                      \
        /* right */                                                            \
        ix = mid + 1;                                                          \
      }                                                                        \
    }                                                                          \
                                                                               \
    return ix;                                                                 \
  }                                                                            \
                                                                               \
  typedef Option(T) A_NAME##Pop;                                               \
private                                                                        \
  A_NAME##Pop A_NAME##_pop(A_NAME *array) {                                    \
    /* Initialise as not valid */                                              \
    A_NAME##Pop ret = {                                                        \
        .valid = false,                                                        \
    };                                                                         \
                                                                               \
    if (array->len == 0) {                                                     \
      return ret;                                                              \
    }                                                                          \
                                                                               \
    array->len -= 1;                                                           \
                                                                               \
    ret.valid = true;                                                          \
    ret.dat = array->dat[array->len];                                          \
    return ret;                                                                \
  }                                                                            \
                                                                               \
  typedef Option(T) A_NAME##Peek;                                              \
private                                                                        \
  A_NAME##Peek A_NAME##_peek(A_NAME *array) {                                  \
    /* Initialise as not valid */                                              \
    A_NAME##Peek ret = {                                                       \
        .valid = false,                                                        \
    };                                                                         \
                                                                               \
    if (array->len == 0) {                                                     \
      return ret;                                                              \
    }                                                                          \
                                                                               \
    ret.valid = true;                                                          \
    ret.dat = array->dat[array->len - 1];                                      \
    return ret;                                                                \
  }                                                                            \
                                                                               \
  typedef Option(usize) A_NAME##Lookup;                                        \
private                                                                        \
  A_NAME##Lookup A_NAME##_linear_lookup(const A_NAME *array, const T *entry,   \
                                        bool eq(const T *a, const T *b)) {     \
    for (usize i = 0; i < array->len; i++) {                                   \
      if (eq(&array->dat[i], entry)) {                                         \
        return (A_NAME##Lookup){                                               \
            .valid = true,                                                     \
            .dat = i,                                                          \
        };                                                                     \
      }                                                                        \
    }                                                                          \
    return (A_NAME##Lookup){                                                   \
        .valid = false,                                                        \
    };                                                                         \
  }                                                                            \
                                                                               \
private                                                                        \
  void A_NAME##_remove(A_NAME *array, usize ix) {                              \
    assert(ix < array->len);                                                   \
    for (usize i = ix; i < array->len - 1; i++) {                              \
      array->dat[i] = array->dat[i + 1];                                       \
    }                                                                          \
    array->len--;                                                              \
  }                                                                            \
                                                                               \
  void REQUIRE_SEMICOLON()

////////////////////////////////////////////////////////////////////////////////
// Binary Heap

// A Max Heap
// COMP_FUN of the shape: int cmp(const T *a, const T *b);
// comparison function which returns​a negative integer value if the first
// argument is less than the second, a positive integer value if the first
// argument is greater than the second and zero if the arguments are equivalent.
#define define_binary_heap(B_NAME, T, N, COMP_FUN)                             \
  typedef struct {                                                             \
    usize len;                                                                 \
    T dat[N];                                                                  \
  } B_NAME;                                                                    \
                                                                               \
  /* Implementation from https://en.wikipedia.org/wiki/Binary_heap#Insert */   \
private                                                                        \
  void B_NAME##_insert(B_NAME *binary_heap, T x) {                             \
    /* Add the element to the bottom level of the heap at the leftmost open    \
     * space. */                                                               \
    assert(binary_heap->len < N);                                              \
    usize ix = binary_heap->len;                                               \
    binary_heap->dat[ix] = x;                                                  \
    binary_heap->len += 1;                                                     \
                                                                               \
    if (binary_heap->len == 1) {                                               \
      return;                                                                  \
    }                                                                          \
                                                                               \
    /* Compare the added element with its parent; if they are in the correct   \
     * order, stop. */                                                         \
    usize parent = (ix - 1) / 2;                                               \
                                                                               \
    while (true) {                                                             \
      int cmp = COMP_FUN(&binary_heap->dat[parent], &binary_heap->dat[ix]);    \
                                                                               \
      if (cmp >= 0) {                                                          \
        return;                                                                \
      }                                                                        \
                                                                               \
      /* If not, swap the element with its parent and return to the previous   \
       * step. */                                                              \
      swap(&binary_heap->dat[ix], &binary_heap->dat[parent], sizeof(T));       \
                                                                               \
      ix = parent;                                                             \
      if (ix == 0) {                                                           \
        return;                                                                \
      }                                                                        \
                                                                               \
      parent = (ix - 1) / 2;                                                   \
    }                                                                          \
  }                                                                            \
                                                                               \
  typedef Option(T) B_NAME##Extract;                                           \
  /* Implementation from https://en.wikipedia.org/wiki/Binary_heap#Extract */  \
private                                                                        \
  B_NAME##Extract B_NAME##_extract(B_NAME *binary_heap) {                      \
    /* Initialise as not valid */                                              \
    B_NAME##Extract ret = {                                                    \
        .valid = false,                                                        \
    };                                                                         \
                                                                               \
    if (binary_heap->len == 0) {                                               \
      return ret;                                                              \
    }                                                                          \
    ret.valid = true;                                                          \
    ret.dat = binary_heap->dat[0];                                             \
                                                                               \
    binary_heap->len--;                                                        \
    if (binary_heap->len == 0) {                                               \
      return ret;                                                              \
    }                                                                          \
                                                                               \
    /* Replace the root of the heap with the last element on the last level.   \
     */                                                                        \
    binary_heap->dat[0] = binary_heap->dat[binary_heap->len];                  \
                                                                               \
    usize ix = 0;                                                              \
    while (true) {                                                             \
      /* No left child, we're done */                                          \
      if (ix * 2 + 1 >= binary_heap->len) {                                    \
        return ret;                                                            \
      }                                                                        \
                                                                               \
      int cmp_l =                                                              \
          COMP_FUN(&binary_heap->dat[ix], &binary_heap->dat[ix * 2 + 1]);      \
                                                                               \
      /* No right child, check with left if valid state, then we're done */    \
      if (ix * 2 + 2 >= binary_heap->len) {                                    \
        if (cmp_l < 0) {                                                       \
          swap(&binary_heap->dat[ix], &binary_heap->dat[ix * 2 + 1],           \
               sizeof(T));                                                     \
        }                                                                      \
        return ret;                                                            \
      }                                                                        \
                                                                               \
      int cmp_r =                                                              \
          COMP_FUN(&binary_heap->dat[ix], &binary_heap->dat[ix * 2 + 2]);      \
                                                                               \
      /* Compare the new root with its children; if they are in the correct    \
       * order, stop. */                                                       \
      if (cmp_l >= 0 && cmp_r >= 0) {                                          \
        return ret;                                                            \
      }                                                                        \
                                                                               \
      /* If not, swap the element with one of its children and return to the   \
       * previous step. */                                                     \
      int cmp_c = COMP_FUN(&binary_heap->dat[ix * 2 + 1],                      \
                           &binary_heap->dat[ix * 2 + 2]);                     \
                                                                               \
      if (cmp_c < 0) {                                                         \
        /* Right is greater */                                                 \
        swap(&binary_heap->dat[ix], &binary_heap->dat[ix * 2 + 2], sizeof(T)); \
        ix = ix * 2 + 2;                                                       \
                                                                               \
      } else {                                                                 \
        /* Left is greater */                                                  \
        swap(&binary_heap->dat[ix], &binary_heap->dat[ix * 2 + 1], sizeof(T)); \
        ix = ix * 2 + 1;                                                       \
      }                                                                        \
    }                                                                          \
  }                                                                            \
                                                                               \
  void REQUIRE_SEMICOLON()

////////////////////////////////////////////////////////////////////////////////
// Span

typedef struct {
  const u8 *dat;
  usize len;
} Span;

private
Span Span_from_str(const char *str) {
  return (Span){
      .dat = (u8 *)str,
      .len = strlen(str),
  };
}

// Load a file and get a Span to its content
//
// Note: This function leaks a file descriptor and doesn't munmap for you
private
Span Span_from_file(const char *path) {
  i32 fd = sys_open(path, O_RDONLY, 0);
  assert_msg(fd > 0, "error opening file");

  isize len = sys_lseek(fd, 0, SEEK_END);
  assert(len >= 0);

  u8 *dat = (u8 *)sys_mmap(NULL, (usize)len, PROT_READ, MAP_PRIVATE, fd, 0);
  assert(dat != (void *)-1);

  return (Span){
      .dat = dat,
      .len = (usize)len,
  };
}

private
Hash Span_hash(const Span *span) {
  FxHasher hasher = {0};

  for (usize i = 0; i < span->len; i++) {
    FxHasher_add(&hasher, (usize)span->dat[i]);
  }

  return hasher;
}

// Intended for generic equality
private
bool Span_eq(const Span *a, const Span *b) {
  return a->len == b->len && memcmp(a->dat, b->dat, a->len) == 0;
}

private
bool Span_match(const Span *x, const char *ref) {
  Span y = Span_from_str(ref);
  return Span_eq(x, &y);
}

// Slice x[from..to] which is inclusive from and exclusive to
private
Span Span_slice(Span x, usize from, usize to) {
  assert(from <= to);
  assert(to <= x.len);

  if (from == to) {
    return (Span){0};
  }

  return (Span){
      .dat = x.dat + from,
      .len = to - from,
  };
}

typedef Option(T2(u64, Span)) SpanParseU64;
private
SpanParseU64 Span_parse_u64(Span x, u8 base) {
  usize len = x.len;
  u64 res = parse_u64(x.dat, &len, base);

  if (len == 0) {
    return (SpanParseU64){
        .valid = false,
    };
  }

  return (SpanParseU64){
      .dat =
          {
              .fst = res,
              .snd = Span_slice(x, len, x.len),
          },
      .valid = true,
  };
}

typedef Option(T2(i64, Span)) SpanParseI64;
private
SpanParseI64 Span_parse_i64(Span x, u8 base) {
  usize len = x.len;
  i64 res = parse_i64(x.dat, &len, base);

  if (len == 0) {
    return (SpanParseI64){
        .valid = false,
    };
  }

  return (SpanParseI64){
      .dat =
          {
              .fst = res,
              .snd = Span_slice(x, len, x.len),
          },
      .valid = true,
  };
}

private
inline bool Span_starts_with(Span x, Span start) {
  return start.len <= x.len && memcmp(x.dat, start.dat, start.len) == 0;
}

private
inline bool Span_starts_with_str(Span x, const char *start) {
  usize len = strlen(start);
  return len <= x.len && memcmp(x.dat, start, len) == 0;
}

private
inline Span Span_trim_start_whitespace(Span x) {
  usize offset = 0;

  while (offset < x.len && (x.dat[offset] == ' ' || x.dat[offset] == '\n' ||
                            x.dat[offset] == '\t')) {
    offset++;
  }

  return Span_slice(x, offset, x.len);
}

private
inline Span Span_trim_end_whitespace(Span x) {
  usize offset = x.len;

  while (offset > 0 && (x.dat[offset - 1] == ' ' || x.dat[offset - 1] == '\n' ||
                        x.dat[offset - 1] == '\t')) {
    offset--;
  }

  return Span_slice(x, 0, offset);
}

private
inline Span Span_trim_start(Span x, Span start) {
  if (Span_starts_with(x, start)) {
    return Span_slice(x, start.len, x.len);
  }

  return x;
}

typedef Option(T2(Span, Span)) SpanSplitOn;
private
SpanSplitOn Span_split_on(u8 byte, Span x) {
  const u8 *match = (const u8 *)memchr(x.dat, byte, x.len);
  usize match_ix = (usize)(match - x.dat);

  if (match == NULL) {
    return (SpanSplitOn){
        .valid = false,
    };
  } else {
    Span fst = Span_slice(x, 0, match_ix);
    Span snd = Span_slice(x, match_ix + 1, x.len);
    return (SpanSplitOn){
        .dat =
            {
                .fst = fst,
                .snd = snd,
            },
        .valid = true,
    };
  }
}

typedef struct {
  Span rest;
  u8 sep;
} SpanSplitIterator;

private
SpanSplitIterator Span_split_lines(Span x) {
  return (SpanSplitIterator){
      .rest = x,
      .sep = (u8)'\n',
  };
}

private
SpanSplitIterator Span_split_words(Span x) {
  return (SpanSplitIterator){
      .rest = x,
      .sep = (u8)' ',
  };
}

typedef Option(Span) SpanSplitIteratorNext;
private
SpanSplitIteratorNext SpanSplitIterator_next(SpanSplitIterator *it) {
  if (it->rest.len == 0) {
    return (SpanSplitIteratorNext){
        .valid = false,
    };
  }

  SpanSplitOn split = Span_split_on(it->sep, it->rest);

  if (split.valid) {
    SpanSplitIteratorNext ret = {
        .dat = split.dat.fst,
        .valid = true,
    };
    it->rest = split.dat.snd;
    return ret;
  } else {
    SpanSplitIteratorNext ret = {
        .dat = it->rest,
        .valid = true,
    };
    it->rest.len = 0;
    return ret;
  }
}

private
void SpanSplitIterator_skip(SpanSplitIterator *it, usize count) {
  for (usize i = 0; i < count; i++) {
    UNWRAP(SpanSplitIterator_next(it));
  }
}

////////////////////////////////////////////////////////////////////////////////
// HashMap

// Define a hash_map with N elements, keys K, values V
// K_HASH is a function: Hash func(const K *key)
// K_EQ is a function: bool func(const K *a, const K *b)
#define define_hash_map(H_NAME, K, V, N, K_HASH, K_EQ)                         \
  typedef struct {                                                             \
    usize count;                                                               \
    bool occupied[N];                                                          \
    K keys[N];                                                                 \
    V values[N];                                                               \
  } H_NAME;                                                                    \
                                                                               \
  const usize H_NAME##_capacity = N;                                           \
                                                                               \
private                                                                        \
  usize H_NAME##_entry_ix(const H_NAME *hm, const K *key) {                    \
    Hash hash = K_HASH(key);                                                   \
                                                                               \
    usize start_ix = hash % N;                                                 \
                                                                               \
    usize ix = start_ix;                                                       \
    while (hm->occupied[ix] && !K_EQ(&hm->keys[ix], key)) {                    \
      ix = (ix + 1) % N;                                                       \
                                                                               \
      assert(ix != start_ix); /* Ran out of space */                           \
    }                                                                          \
                                                                               \
    return ix;                                                                 \
  }                                                                            \
                                                                               \
private                                                                        \
  inline bool H_NAME##_contains(const H_NAME *hm, const K *key) {              \
    usize ix = H_NAME##_entry_ix(hm, key);                                     \
    return hm->occupied[ix];                                                   \
  }                                                                            \
                                                                               \
  typedef Option(V *) H_NAME##Lookup;                                          \
private                                                                        \
  H_NAME##Lookup H_NAME##_lookup(H_NAME *hm, const K *key) {                   \
    usize ix = H_NAME##_entry_ix(hm, key);                                     \
    if (hm->occupied[ix]) {                                                    \
      return (H_NAME##Lookup){                                                 \
          .dat = &hm->values[ix],                                              \
          .valid = true,                                                       \
      };                                                                       \
    } else {                                                                   \
      return (H_NAME##Lookup){                                                 \
          .valid = false,                                                      \
      };                                                                       \
    }                                                                          \
  }                                                                            \
                                                                               \
  /* Return if it is overwriting a previous entry */                           \
private                                                                        \
  bool H_NAME##_insert(H_NAME *hm, K key, V value) {                           \
    usize ix = H_NAME##_entry_ix(hm, &key);                                    \
    bool was_occupied = hm->occupied[ix];                                      \
                                                                               \
    hm->occupied[ix] = true;                                                   \
    hm->keys[ix] = key;                                                        \
    hm->values[ix] = value;                                                    \
                                                                               \
    if (!was_occupied) {                                                       \
      hm->count += 1;                                                          \
    }                                                                          \
                                                                               \
    return was_occupied;                                                       \
  }                                                                            \
                                                                               \
private                                                                        \
  V *H_NAME##_insert_modify(H_NAME *hm, K key, V def) {                        \
    usize ix = H_NAME##_entry_ix(hm, &key);                                    \
    bool was_occupied = hm->occupied[ix];                                      \
                                                                               \
    hm->occupied[ix] = true;                                                   \
    hm->keys[ix] = key;                                                        \
                                                                               \
    if (!was_occupied) {                                                       \
      hm->count += 1;                                                          \
      hm->values[ix] = def;                                                    \
    }                                                                          \
                                                                               \
    return &hm->values[ix];                                                    \
  }                                                                            \
                                                                               \
  /* Implementation based on https://en.wikipedia.org/wiki/Open_addressing */  \
  typedef Option(T2(K, V)) H_NAME##Remove;                                     \
private                                                                        \
  H_NAME##Remove H_NAME##_remove(H_NAME *hm, const K *key) {                   \
    usize i = H_NAME##_entry_ix(hm, key);                                      \
    H_NAME##Remove ret = {                                                     \
        .valid = false,                                                        \
    };                                                                         \
                                                                               \
    if (!hm->occupied[i]) {                                                    \
      return ret;                                                              \
    }                                                                          \
                                                                               \
    ret.valid = true;                                                          \
    ret.dat.fst = hm->keys[i];                                                 \
    ret.dat.snd = hm->values[i];                                               \
                                                                               \
    hm->count -= 1;                                                            \
    hm->occupied[i] = false;                                                   \
                                                                               \
    usize j = i;                                                               \
    while (true) {                                                             \
      j = (j + 1) % N;                                                         \
                                                                               \
      if (!hm->occupied[j]) {                                                  \
        return ret;                                                            \
      }                                                                        \
                                                                               \
      usize k = K_HASH(&hm->keys[j]) % N;                                      \
                                                                               \
      /* determine if k lies cyclically in (i,j]                               \
         i ≤ j: |    i..k..j    |                                            \
         i > j: |.k..j     i....| or |....j     i..k.| */                      \
      if (i <= j) {                                                            \
        if ((i < k) && (k <= j)) {                                             \
          continue;                                                            \
        }                                                                      \
      } else {                                                                 \
        if ((i < k) || (k <= j)) {                                             \
          continue;                                                            \
        }                                                                      \
      }                                                                        \
                                                                               \
      hm->keys[i] = hm->keys[j];                                               \
      hm->values[i] = hm->values[j];                                           \
      hm->occupied[j] = false;                                                 \
      i = j;                                                                   \
    }                                                                          \
  }                                                                            \
                                                                               \
  void REQUIRE_SEMICOLON()

////////////////////////////////////////////////////////////////////////////////
// BitSet

#define define_bit_set(B_NAME, T, N)                                           \
  typedef struct {                                                             \
    T dat[N];                                                                  \
  } B_NAME;                                                                    \
                                                                               \
private                                                                        \
  usize T##_bits = 8 * sizeof(T);                                              \
                                                                               \
private                                                                        \
  usize B_NAME##_size = N * 8 * sizeof(T);                                     \
                                                                               \
private                                                                        \
  inline void B_NAME##_insert(B_NAME *bs, usize x) {                           \
    assert(x / T##_bits < N);                                                  \
    bs->dat[x / T##_bits] |= ((T)1) << (x % T##_bits);                         \
  }                                                                            \
                                                                               \
private                                                                        \
  void B_NAME##_set_many(B_NAME *bs, usize x) {                                \
    assert(x / T##_bits < N);                                                  \
    bs->dat[x / T##_bits] |= (((T)1) << (x % T##_bits)) - 1;                   \
    for (usize i = 0; i < x / T##_bits; i++) {                                 \
      bs->dat[i] = ~((T)0);                                                    \
    }                                                                          \
  }                                                                            \
                                                                               \
private                                                                        \
  inline bool B_NAME##_contains(B_NAME bs, usize x) {                          \
    assert(x / T##_bits < N);                                                  \
    return (bool)((bs.dat[x / T##_bits] >> (x % T##_bits)) & 1);               \
  }                                                                            \
                                                                               \
private                                                                        \
  inline void B_NAME##_remove(B_NAME *bs, usize x) {                           \
    assert(x / T##_bits < N);                                                  \
    bs->dat[x / T##_bits] &= (T) ~(((T)1) << (x % T##_bits));                  \
  }                                                                            \
                                                                               \
private                                                                        \
  bool B_NAME##_eq(B_NAME a, B_NAME b) {                                       \
    for (usize i = 0; i < N; i++) {                                            \
      if (a.dat[i] != b.dat[i]) {                                              \
        return false;                                                          \
      }                                                                        \
    }                                                                          \
    return true;                                                               \
  }                                                                            \
                                                                               \
private                                                                        \
  B_NAME B_NAME##_union(B_NAME a, B_NAME b) {                                  \
    for (usize i = 0; i < N; i++) {                                            \
      a.dat[i] |= b.dat[i];                                                    \
    }                                                                          \
    return a;                                                                  \
  }                                                                            \
                                                                               \
private                                                                        \
  B_NAME B_NAME##_intersection(B_NAME a, B_NAME b) {                           \
    for (usize i = 0; i < N; i++) {                                            \
      a.dat[i] &= b.dat[i];                                                    \
    }                                                                          \
    return a;                                                                  \
  }                                                                            \
                                                                               \
  /* Is "a" a subset of "b" */                                                 \
private                                                                        \
  inline bool B_NAME##_is_subset(B_NAME a, B_NAME b) {                         \
    return B_NAME##_eq(a, B_NAME##_intersection(a, b));                        \
  }                                                                            \
                                                                               \
private                                                                        \
  inline usize B_NAME##_count(B_NAME a) {                                      \
    usize x = 0;                                                               \
    for (usize i = 0; i < N; i++) {                                            \
      x += (usize)__builtin_popcountl((u64)a.dat[i]);                          \
    }                                                                          \
    return x;                                                                  \
  }                                                                            \
                                                                               \
  void REQUIRE_SEMICOLON()

////////////////////////////////////////////////////////////////////////////////
// String

define_array(String, u8, 256);

private
inline void String_clear(String *str) { str->len = 0; }

private
void String_push_str(String *str, const char *s) {
  usize len = strlen(s);
  assert(str->len + len <= String_capacity);
  memcpy(&str->dat[str->len], s, len);
  str->len += len;
}

private
void String_push_span(String *str, Span spn) {
  assert(str->len + spn.len <= String_capacity);
  memcpy(&str->dat[str->len], spn.dat, spn.len);
  str->len += spn.len;
}

private
void String_push_u64(String *str, u64 x, u8 base) {
  u8 buf[128];
  usize len = fmt_u64(buf, 128, x, base);
  assert(str->len + len <= String_capacity);
  memcpy(&str->dat[str->len], buf, len);
  str->len += len;
}

private
void String_push_i64(String *str, i64 x, u8 base) {
  u8 buf[128];
  usize len = fmt_i64(buf, 128, x, base);
  assert(str->len + len <= String_capacity);
  memcpy(&str->dat[str->len], buf, len);
  str->len += len;
}

private
inline void String_print(const String *str) {
  sys_write(STDOUT, str->dat, str->len);
}

private
inline void String_println(String *str) {
  String_push(str, '\n');
  String_print(str);
}

private
inline void String_printlnc(String *str) {
  String_push(str, '\n');
  String_print(str);
  String_clear(str);
}

typedef struct {
  const void *arg;
  usize size;
} PrintfArg;

private
void __printf(const char *fmt, usize argc, const PrintfArg *argv) {
  String out = {0};
  usize len = strlen(fmt);
  usize arg_ix = 0;
  for (usize i = 0; i < len; i++) {
    if (fmt[i] == '%') {
      i++;
      assert_msg(i < len, "printf: fmt str too short");
      assert_msg(arg_ix < argc, "printf: fmt string has too many args");
      PrintfArg p_arg = argv[arg_ix];

      switch (fmt[i]) {
      case 's':
        if (p_arg.size == sizeof(Span)) {
          String_push_span(&out, *((Span *)p_arg.arg));
        } else if (p_arg.size == sizeof(const char *)) {
          String_push_str(&out, *((const char **)p_arg.arg));
        } else if (p_arg.size == sizeof(String)) {
          String x = *((String *)p_arg.arg);
          String_push_span(&out, (Span){.dat = x.dat, .len = x.len});
        } else {
          panic("Unexpected arg for s");
        }
        arg_ix++;
        break;
      case 'i':
        switch (p_arg.size) {
        case 1:
          String_push_i64(&out, (i64) * ((i8 *)p_arg.arg), 10);
          break;
        case 2:
          String_push_i64(&out, (i64) * ((i16 *)p_arg.arg), 10);
          break;
        case 4:
          String_push_i64(&out, (i64) * ((i32 *)p_arg.arg), 10);
          break;
        case 8:
          String_push_i64(&out, *((i64 *)p_arg.arg), 10);
          break;
        default:
          panic("Unexpected arg size");
        }
        arg_ix++;
        break;
      case 'u':
        switch (p_arg.size) {
        case 1:
          String_push_u64(&out, (u64) * ((u8 *)p_arg.arg), 10);
          break;
        case 2:
          String_push_u64(&out, (u64) * ((u16 *)p_arg.arg), 10);
          break;
        case 4:
          String_push_u64(&out, (u64) * ((u32 *)p_arg.arg), 10);
          break;
        case 8:
          String_push_u64(&out, *((u64 *)p_arg.arg), 10);
          break;
        default:
          panic("Unexpected arg size");
        }
        arg_ix++;
        break;
      case 'x':
        switch (p_arg.size) {
        case 1:
          String_push_u64(&out, (u64) * ((u8 *)p_arg.arg), 16);
          break;
        case 2:
          String_push_u64(&out, (u64) * ((u16 *)p_arg.arg), 16);
          break;
        case 4:
          String_push_u64(&out, (u64) * ((u32 *)p_arg.arg), 16);
          break;
        case 8:
          String_push_u64(&out, *((u64 *)p_arg.arg), 16);
          break;
        default:
          panic("Unexpected arg size");
        }
        arg_ix++;
        break;
      case '%':
        String_push(&out, '%');
        break;
      default:
        panic("Unexpected printf fmt char");
      }
    } else {
      String_push(&out, (u8)fmt[i]);
    }
  }

  String_print(&out);
}

#define printf0(fmt) __printf(fmt, 0, NULL);
#define printf1(fmt, A)                                                        \
  ({                                                                           \
    __auto_type PRINTF_a = A;                                                  \
    __printf(fmt, 1,                                                           \
             (PrintfArg[]){(PrintfArg){.arg = (const void *)&PRINTF_a,         \
                                       .size = sizeof(PRINTF_a)}});            \
  })
#define printf2(fmt, A, B)                                                     \
  ({                                                                           \
    __auto_type PRINTF_a = A;                                                  \
    __auto_type PRINTF_b = B;                                                  \
    __printf(fmt, 2,                                                           \
             (PrintfArg[]){(PrintfArg){.arg = (const void *)&PRINTF_a,         \
                                       .size = sizeof(PRINTF_a)},              \
                           (PrintfArg){.arg = (const void *)&PRINTF_b,         \
                                       .size = sizeof(PRINTF_b)}});            \
  })
#define printf3(fmt, A, B, C)                                                  \
  ({                                                                           \
    __auto_type PRINTF_a = A;                                                  \
    __auto_type PRINTF_b = B;                                                  \
    __auto_type PRINTF_c = C;                                                  \
    __printf(fmt, 3,                                                           \
             (PrintfArg[]){(PrintfArg){.arg = (const void *)&PRINTF_a,         \
                                       .size = sizeof(PRINTF_a)},              \
                           (PrintfArg){.arg = (const void *)&PRINTF_b,         \
                                       .size = sizeof(PRINTF_b)},              \
                           (PrintfArg){.arg = (const void *)&PRINTF_c,         \
                                       .size = sizeof(PRINTF_c)}});            \
  })
#define printf4(fmt, A, B, C, D)                                               \
  ({                                                                           \
    __auto_type PRINTF_a = A;                                                  \
    __auto_type PRINTF_b = B;                                                  \
    __auto_type PRINTF_c = C;                                                  \
    __auto_type PRINTF_d = D;                                                  \
    __printf(fmt, 4,                                                           \
             (PrintfArg[]){(PrintfArg){.arg = (const void *)&PRINTF_a,         \
                                       .size = sizeof(PRINTF_a)},              \
                           (PrintfArg){.arg = (const void *)&PRINTF_b,         \
                                       .size = sizeof(PRINTF_b)},              \
                           (PrintfArg){.arg = (const void *)&PRINTF_c,         \
                                       .size = sizeof(PRINTF_c)},              \
                           (PrintfArg){.arg = (const void *)&PRINTF_d,         \
                                       .size = sizeof(PRINTF_c)}});            \
  })

// Better error message now that we can format __LINE__ properly
static void print_msg_with_loc(const char *file, u64 line, const char *msg,
                               usize msg_len) {
  Span msg_span = {
      .dat = (const u8 *)msg,
      .len = msg_len,
  };
  printf3("\033[31;1;4m%s:%u\033[0m: %s\n", file, line, msg_span);
}

#endif // BAZ_HEADER
