CC      ?= clang
CFLAGS  ?= -O2 -Wall -Wextra -Wpedantic -std=c11 -Iinclude
DEBUG_FLAGS ?= -O0 -g

# On macOS, build for the host arch (x86_64 or arm64).
# On Linux, no -arch flag.
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    ARCH := -arch $(shell uname -m)
else
    ARCH :=
endif

BUILD   := build
LIB     := $(BUILD)/libarena.a

SRCS    := $(wildcard src/*.c)
OBJS    := $(patsubst src/%.c,$(BUILD)/%.o,$(SRCS))
HEADERS := $(wildcard include/*.h)

# ---- library --------------------------------------------------------

all: $(LIB) $(BUILD)/test_arena $(BUILD)/bench_arena

$(BUILD):
	@mkdir -p $@

$(BUILD)/%.o: src/%.c $(HEADERS) | $(BUILD)
	$(CC) $(ARCH) $(CFLAGS) -c $< -o $@

$(LIB): $(OBJS)
	ar rcs $@ $^

# ---- tests ----------------------------------------------------------

$(BUILD)/test_arena: tests/test_arena.c $(LIB) $(HEADERS) | $(BUILD)
	$(CC) $(ARCH) $(CFLAGS) -Itests tests/test_arena.c $(LIB) -o $@

test: $(BUILD)/test_arena
	./$(BUILD)/test_arena

# ---- benchmark ------------------------------------------------------

$(BUILD)/bench_arena: bench/bench_arena.c $(LIB) $(HEADERS) | $(BUILD)
	$(CC) $(ARCH) $(CFLAGS) -Ibench bench/bench_arena.c $(LIB) -o $@

bench: $(BUILD)/bench_arena
	./$(BUILD)/bench_arena $(N)

# ---- examples -------------------------------------------------------

$(BUILD)/parse_demo: examples/parse_demo.c $(LIB) $(HEADERS) | $(BUILD)
	$(CC) $(ARCH) $(CFLAGS) -Iexamples examples/parse_demo.c $(LIB) -o $@

parse-demo: $(BUILD)/parse_demo
	@./$(BUILD)/parse_demo
	@echo
	@./$(BUILD)/parse_demo "int x = 42;  // a comment"

# ---- debug build ----------------------------------------------------

debug: CFLAGS := $(DEBUG_FLAGS) -Wall -Wextra -Wpedantic -std=c11 -Iinclude
debug: clean all

# ---- housekeeping ---------------------------------------------------

clean:
	rm -rf $(BUILD)

.PHONY: all test bench parse-demo debug clean
