CC := gcc
CORTEX_ROOT ?= ..
CFLAGS := -Wall -Wextra -std=c11 -I. -I$(CORTEX_ROOT) -I$(CORTEX_ROOT)/core -I$(CORTEX_ROOT)/action -I$(CORTEX_ROOT)/config
APP_SRCS := main.c config/routes.c $(wildcard app/controllers/*.c) $(wildcard app/models/*.c) $(wildcard app/neural/*.c) $(wildcard app/neural/*/*.c)

# Match root Makefile: link libpq when available so `-lcortex` resolves PQ* symbols
# if libcortex.a was built with PostgreSQL support.
HAVE_PG := $(shell pkg-config --exists libpq 2>/dev/null && echo yes)
ifeq ($(HAVE_PG),yes)
  PG_LDFLAGS := $(shell pkg-config --libs libpq)
endif

cortex_app: $(APP_SRCS)
	$(CC) $(CFLAGS) $(APP_SRCS) -L$(CORTEX_ROOT) -lcortex -lm $(PG_LDFLAGS) -o cortex_app

.PHONY: all clean server dev assets-build
all: cortex_app
server: cortex_app
	./cortex_app
assets-build:
	./$(CORTEX_ROOT)/cortex assets:build
dev:
	./$(CORTEX_ROOT)/cortex dev
clean:
	rm -f cortex_app
