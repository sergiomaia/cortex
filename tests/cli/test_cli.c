#include "../test_assert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../cli/cli.h"

static char *dup_cstr(const char *s) {
    size_t n = strlen(s) + 1;
    char *out = (char *)malloc(n);
    if (!out) {
        return NULL;
    }
    memcpy(out, s, n);
    return out;
}

static char **make_argv(const char *a0, const char *a1, const char *a2, const char *a3, int *out_argc) {
    int argc = 0;
    char **argv = (char **)calloc(5, sizeof(char *));
    if (!argv) {
        *out_argc = 0;
        return NULL;
    }
    if (a0) argv[argc++] = dup_cstr(a0);
    if (a1) argv[argc++] = dup_cstr(a1);
    if (a2) argv[argc++] = dup_cstr(a2);
    if (a3) argv[argc++] = dup_cstr(a3);
    *out_argc = argc;
    return argv;
}

static void free_argv(char **argv, int argc) {
    int i;
    if (!argv) {
        return;
    }
    for (i = 0; i < argc; ++i) {
        free(argv[i]);
    }
    free(argv);
}

void test_cli_parse_server_command(void) {
    int argc;
    char **argv = make_argv("cortex", "server", NULL, NULL, &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_SERVER);
    ASSERT_TRUE(parsed.name == NULL);

    free_argv(argv, argc);
}

void test_cli_parse_generate_controller_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "controller", "incident", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_CONTROLLER);
    ASSERT_STR_EQ(parsed.name, "incident");

    free_argv(argv, argc);
}

void test_cli_parse_invalid_command_fails(void) {
    int argc;
    char **argv = make_argv("cortex", "unknown", NULL, NULL, &argc);
    CliParsed parsed;

    ASSERT_TRUE(cli_parse(argc, argv, &parsed) != 0);

    free_argv(argv, argc);
}

/* Integration test for handler execution: generate controller. */
static int file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return 0;
    }
    fclose(f);
    return 1;
}

static void remove_if_exists(const char *path) {
    if (file_exists(path)) {
        remove(path);
    }
}

void test_cli_dispatch_generate_controller_executes_handler(void) {
    const char *path = "app/controllers/incidents_controller.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_CONTROLLER;
    parsed.name = "incidents";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

