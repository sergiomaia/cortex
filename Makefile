CC := gcc
VERSION := $(strip $(file <VERSION))
CFLAGS := -Wall -Wextra -std=c11 -I. -Icore -Iaction -Iflow -Icache -Iguard -Iforge -Iconfig -DCORTEX_VERSION=\"$(VERSION)\" -DCORTEX_SOURCE_ROOT=\"$(CURDIR)\"

CORE_SRCS := $(wildcard core/*.c)
ACTION_SRCS := $(wildcard action/*.c)
FLOW_SRCS := $(wildcard flow/*.c)
CACHE_SRCS := $(wildcard cache/*.c)
GUARD_SRCS := $(wildcard guard/*.c)
FORGE_SRCS := $(wildcard forge/*.c)
DB_SRCS := $(wildcard db/*.c)
APP_SRCS := $(wildcard app/*/*.c) $(wildcard app/*/*/*.c)
CONFIG_SRCS := $(wildcard config/*.c)

# Provide an executable entrypoint, but keep the static library unchanged.
# `cli/cortex_main.c` is linked into the `cortex` binary only (not archived into libcortex.a).
CLI_MAIN_SRC := cli/cortex_main.c
CLI_SRCS := $(filter-out $(CLI_MAIN_SRC), $(wildcard cli/*.c))

SRCS := $(CORE_SRCS) $(ACTION_SRCS) $(FLOW_SRCS) $(CACHE_SRCS) $(GUARD_SRCS) $(FORGE_SRCS) $(DB_SRCS) $(CLI_SRCS) $(APP_SRCS) $(CONFIG_SRCS)
OBJS := $(SRCS:.c=.o)

TEST_SRCS := tests/test_runner.c \
             tests/flow/test_flow.c \
             tests/core/test_core_app.c \
             tests/core/test_core_logger.c \
             tests/core/test_core_config.c \
             tests/core/test_routes.c \
             tests/core/test_neural_runtime.c \
             tests/core/test_neural_prompt.c \
             tests/core/test_incident_summary.c \
             tests/core/test_neural_memory.c \
             tests/core/test_pulse_log.c \
             tests/core/test_pulse_logger.c \
             tests/core/test_pulse_metrics.c \
             tests/core/test_pulse_trace.c \
             tests/core/test_pulse_ai.c \
             tests/core/test_neural_pulse_ai_integration.c \
             tests/core/test_neural_stream.c \
             tests/core/test_neural_embedding.c \
             tests/core/test_neural_retrieval.c \
             tests/core/test_llm_integration.c \
             tests/core/test_neural_agent.c \
             tests/core/test_active_model.c \
             tests/core/test_active_migration.c \
             tests/core/test_active_record.c \
             tests/core/test_active_query.c \
             tests/core/test_active_relations.c \
             tests/action/test_action_request_response.c \
             tests/action/test_action_router.c \
             tests/action/test_action_controller.c \
             tests/action/test_action_health.c \
             tests/action/test_action_incidents.c \
             tests/action/test_action_ai_incident_summary.c \
             tests/action/test_action_ai_rag.c \
             tests/cache/test_cache_memory.c \
             tests/guard/test_guard.c \
             tests/forge/test_forge_generators.c \
             tests/db/test_db_create.c \
             tests/db/test_db_migrate.c \
             tests/db/test_db_migration_generator.c \
             tests/cli/test_cli.c \
             tests/cli/test_cli_server.c
TEST_BIN := tests/test_runner

EXE := cortex
LIB := libcortex.a
MAIN_OBJ := $(CLI_MAIN_SRC:.c=.o)

.PHONY: all clean test

all: $(LIB) $(EXE)

$(LIB): $(OBJS)
	@rm -f $(LIB)
	ar rcs $(LIB) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_BIN): $(LIB) $(TEST_SRCS)
	$(CC) $(CFLAGS) $(TEST_SRCS) -L. -lcortex -lm -o $(TEST_BIN)

$(EXE): $(LIB) $(MAIN_OBJ)
	$(CC) $(CFLAGS) $(MAIN_OBJ) -L. -lcortex -lm -o $(EXE)

test: $(TEST_BIN)
	./$(TEST_BIN)

clean:
	rm -f $(OBJS) $(LIB) $(TEST_BIN) $(EXE) $(MAIN_OBJ)

