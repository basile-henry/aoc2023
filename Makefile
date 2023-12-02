CFLAGS = -g -O3 -std=c99 -march=skylake \
	-ffreestanding -nostdlib -nostdinc \
	-Wimplicit-fallthrough \
	-Wall -Wextra -Wconversion -Werror

DAYS = day01 day02

day01: src/day01.c
	$(CC) $(CFLAGS) src/day01.c -o day01

day02: src/day02.c
	$(CC) $(CFLAGS) src/day02.c -o day02

.PHONY: all
all: $(DAYS)

.PHONY: run-all
run-all: all
	@for day in $(DAYS); do echo -e "\033[0;31m$$day:\033[0m"; ./$$day; echo ; done

.PHONY: clean
clean:
	@for day in $(DAYS); do rm -f ./$$day ; done

DAY?=day01

.PHONY: watch
watch:
	@watchexec -c -w src 'rm -f $(DAY) && $(MAKE) $(DAY) && ./$(DAY)'
