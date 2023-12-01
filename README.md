# Advent of Code 2023

This is an attempt to do AoC in C without libc.

Note: This *ONLY* works for Linux x86-64 (targeting `skylake` at the moment)!

# Build and run

Requires a C compiler (I've tried with both GCC and Clang), a `nix-shell` is
provided for convenience.

To build and run all the days so far:
```sh
make run-all
```

To build a single day:
```sh
make day<day>
```

So for day 1

```sh
make day01
```

# License

[MIT - Copyright 2023 Basile Henry](./LICENSE)
