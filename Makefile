CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -Icore -Iaction

CORE_SRCS := $(wildcard core/*.c)
ACTION_SRCS := $(wildcard action/*.c)
SRCS := $(CORE_SRCS) $(ACTION_SRCS)
OBJS := $(SRCS:.c=.o)

TEST_SRCS := tests/test_runner.c
TEST_BIN := tests/test_runner

LIB := libcortex.a

.PHONY: all clean test

all: $(LIB)

$(LIB): $(OBJS)
	@rm -f $(LIB)
	ar rcs $(LIB) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(LIB) $(TEST_SRCS)
	$(CC) $(CFLAGS) $(TEST_SRCS) -L. -lcortex -o $(TEST_BIN)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJS) $(LIB) $(TEST_BIN)

