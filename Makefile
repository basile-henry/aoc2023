CFLAGS = -g -O3 -std=c99 -march=skylake \
	-ffreestanding -nostdlib -nostdinc \
	-Wimplicit-fallthrough \
	-Wall -Wextra -Wconversion -Werror

DAYS = day01 day02 day03 day04 day05 day06 day07 day08

day01: src/day01.c
	$(CC) $(CFLAGS) src/day01.c -o day01

day02: src/day02.c
	$(CC) $(CFLAGS) src/day02.c -o day02

day03: src/day03.c
	$(CC) $(CFLAGS) src/day03.c -o day03

day04: src/day04.c
	$(CC) $(CFLAGS) src/day04.c -o day04

day05: src/day05.c
	$(CC) $(CFLAGS) src/day05.c -o day05

day06: src/day06.c
	$(CC) $(CFLAGS) src/day06.c -o day06

day07: src/day07.c
	$(CC) $(CFLAGS) src/day07.c -o day07

day08: src/day08.c
	$(CC) $(CFLAGS) src/day08.c -o day08

test: src/test.c
	$(CC) $(CFLAGS) src/test.c -o test

.PHONY: all
all: $(DAYS)

.PHONY: run-all
run-all: all
	@for day in $(DAYS); do echo -e "\033[0;31m$$day:\033[0m"; ./$$day; echo ; done

.PHONY: clean
clean:
	@for day in test $(DAYS); do rm -f ./$$day ; done

DAY?=day01

.PHONY: watch
watch:
	@watchexec -c -w src 'rm -f $(DAY) && $(MAKE) $(DAY) && ./$(DAY)'
