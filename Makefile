CFLAGS = -g -O3 -std=c99 -march=skylake \
	-ffreestanding -nostdlib -nostdinc \
	-Wimplicit-fallthrough \
	-Wall -Wextra -Wconversion -Werror

day01: src/day01.c
	$(CC) $(CFLAGS) src/day01.c -o day01

day02: src/day02.c
	$(CC) $(CFLAGS) src/day02.c -o day02

.PHONY: all
all: day01 day02

.PHONY: run-all
run-all: all
	./day01; ./day02

.PHONY: clean
clean:
	rm -f ./day01 ./day02

DAY?=day01

.PHONY: watch
watch:
	watchexec -c -w src 'rm -f $(DAY) && $(MAKE) $(DAY) && ./$(DAY)'
