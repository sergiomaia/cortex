CC := gcc
VERSION := $(strip $(file <VERSION))
COVERAGE ?=

# SQLite: always compile the official amalgamation into libcortex.a so apps only need
# `-lcortex -lm` (see scripts/fetch-sqlite-amalgamation.sh; runs automatically on first build).
SQLITE_CFLAGS := -I$(CURDIR)/vendor/sqlite
SQLITE_SRCS := vendor/sqlite/sqlite3.c

CFLAGS := -Wall -Wextra -std=c11 -pthread $(SQLITE_CFLAGS) -I. -Itests -Icore -Iaction -Iflow -Icache -Iguard -Iforge -Iconfig -Idb -DCORTEX_VERSION=\"$(VERSION)\" -DCORTEX_SOURCE_ROOT=\"$(CURDIR)\"
LDFLAGS ?=

ifeq ($(COVERAGE),yes)
CFLAGS += --coverage
LDFLAGS += --coverage
COVERAGE_LIBS := -lgcov
else
COVERAGE_LIBS :=
endif

# Optional PostgreSQL (libpq). When absent, the build omits db/postgres/postgres_adapter.c.
HAVE_PG := $(shell pkg-config --exists libpq 2>/dev/null && echo yes)

ifeq ($(HAVE_PG),yes)
  PG_CFLAGS  := $(shell pkg-config --cflags libpq)
  PG_LDFLAGS := $(shell pkg-config --libs   libpq)
  CFLAGS     += $(PG_CFLAGS) -DCORTEX_HAVE_POSTGRES
  LDFLAGS    += $(PG_LDFLAGS)
  $(info [cortex] PostgreSQL adapter enabled)
else
  $(info [cortex] PostgreSQL adapter disabled — libpq not found)
endif

# The first explicit rule in this file must not steal the default goal from 'all'.
.DEFAULT_GOAL := all

CORE_SRCS := $(wildcard core/*.c)
ACTION_SRCS := $(wildcard action/*.c)
FLOW_SRCS := $(wildcard flow/*.c)
CACHE_SRCS := $(wildcard cache/*.c)
GUARD_SRCS := $(wildcard guard/*.c)
FORGE_SRCS := $(wildcard forge/*.c)
DB_SRCS := $(wildcard db/*.c) $(wildcard db/sqlite/*.c)
ifeq ($(HAVE_PG),yes)
  DB_SRCS += db/postgres/postgres_adapter.c
endif
APP_SRCS := $(wildcard app/*/*.c) $(wildcard app/*/*/*.c)
# Always include config/routes.c (register_routes); wildcard alone omits it if the file is missing.
CONFIG_SRCS := config/routes.c $(filter-out config/routes.c,$(wildcard config/*.c))

# Provide an executable entrypoint, but keep the static library unchanged.
# `cli/cortex_main.c` is linked into the `cortex` binary only (not archived into libcortex.a).
CLI_MAIN_SRC := cli/cortex_main.c
CLI_SRCS := $(filter-out $(CLI_MAIN_SRC), $(wildcard cli/*.c))

SRCS := $(CORE_SRCS) $(ACTION_SRCS) $(FLOW_SRCS) $(CACHE_SRCS) $(GUARD_SRCS) $(FORGE_SRCS) $(DB_SRCS) $(CLI_SRCS) $(APP_SRCS) $(CONFIG_SRCS) $(SQLITE_SRCS)
OBJS := $(SRCS:.c=.o)

# Only when sqlite3.c is missing; do not depend on the script mtime (avoids re-running every build).
vendor/sqlite/sqlite3.c:
	./scripts/fetch-sqlite-amalgamation.sh

$(OBJS): | vendor/sqlite/sqlite3.c

# All test translation units discovered under tests/, linked with libcortex.a.
TEST_SRCS := $(shell find $(CURDIR)/tests -name '*.c' ! -path '*/vendor/*' | LC_ALL=C sort)
TEST_BIN := build/tests/test_runner
TEST_DB := :memory:

EXE := cortex
LIB := libcortex.a
MAIN_OBJ := $(CLI_MAIN_SRC:.c=.o)

.PHONY: all clean rebuild test tests test-watch coverage clean-test vendor-sqlite

all: $(LIB) $(EXE)

# Sequential full rebuild (use this instead of `make clean & make`, which races:
# background clean deletes *.o while the other make is still building).
rebuild: clean
	$(MAKE) all

vendor-sqlite:
	./scripts/fetch-sqlite-amalgamation.sh

$(LIB): $(OBJS)
	@rm -f $(LIB)
	ar rcs $(LIB) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(LIB) $(TEST_SRCS)
	@mkdir -p $(dir $(TEST_BIN))
	$(CC) $(CFLAGS) $(TEST_SRCS) -L. $(LIB) -lm $(LDFLAGS) $(COVERAGE_LIBS) -o $(TEST_BIN)

$(EXE): $(LIB) $(MAIN_OBJ)
	$(CC) $(CFLAGS) $(MAIN_OBJ) -L. -lcortex -lm $(LDFLAGS) $(COVERAGE_LIBS) -o $(EXE)

test: $(TEST_BIN)
	@TEST_DB="$(TEST_DB)" ./$(TEST_BIN)

tests: test

test-watch:
	@{ command -v entr >/dev/null 2>&1 || { printf "entr is required for test-watch\n"; exit 1; }; }
	find . \( -name '*.c' -o -name '*.h' \) ! -path './vendor/*' | entr -c $(MAKE) test

coverage:
	@command -v lcov >/dev/null 2>&1 || { printf "lcov is required (e.g. sudo apt-get install lcov)\n"; exit 1; }
	$(MAKE) clean
	$(MAKE) COVERAGE=yes $(LIB) $(TEST_BIN)
	./$(TEST_BIN)
	lcov --capture --directory "$(CURDIR)" --output-file coverage.info --no-external \
		--exclude '*/vendor/*' \
		--exclude '*/tests/*'
	genhtml coverage.info --output-directory coverage_html --quiet
	@echo "Report: $$(pwd)/coverage_html/index.html"

clean-test:
	rm -f $(TEST_BIN) coverage.info
	rm -rf coverage_html
	find "$(CURDIR)" ! -path '*/vendor/*' \( -name '*.gcda' -o -name '*.gcno' \) -delete 2>/dev/null || true

clean:
	rm -f $(OBJS) $(LIB) $(TEST_BIN) $(EXE) $(MAIN_OBJ)
