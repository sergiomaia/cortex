CC := gcc
CFLAGS := -Wall -Wextra -std=c11 -Icore -Iaction

CORE_SRCS := $(wildcard core/*.c)
ACTION_SRCS := $(wildcard action/*.c)
SRCS := $(CORE_SRCS) $(ACTION_SRCS)
OBJS := $(SRCS:.c=.o)

LIB := libcortex.a

.PHONY: all clean

all: $(LIB)

$(LIB): $(OBJS)
	@rm -f $(LIB)
	ar rcs $(LIB) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(LIB)

