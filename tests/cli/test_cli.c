#include "../test_assert.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "../../cli/cli.h"
#include "../../forge/forge_generators.h"

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

static char **make_argv6(const char *a0, const char *a1, const char *a2, const char *a3, const char *a4, const char *a5, int *out_argc) {
    int argc = 0;
    char **argv = (char **)calloc(7, sizeof(char *));
    if (!argv) {
        *out_argc = 0;
        return NULL;
    }
    if (a0) argv[argc++] = dup_cstr(a0);
    if (a1) argv[argc++] = dup_cstr(a1);
    if (a2) argv[argc++] = dup_cstr(a2);
    if (a3) argv[argc++] = dup_cstr(a3);
    if (a4) argv[argc++] = dup_cstr(a4);
    if (a5) argv[argc++] = dup_cstr(a5);
    *out_argc = argc;
    return argv;
}

static char **make_argv7(const char *a0, const char *a1, const char *a2, const char *a3, const char *a4, const char *a5, const char *a6, int *out_argc) {
    int argc = 0;
    char **argv = (char **)calloc(8, sizeof(char *));
    if (!argv) {
        *out_argc = 0;
        return NULL;
    }
    if (a0) argv[argc++] = dup_cstr(a0);
    if (a1) argv[argc++] = dup_cstr(a1);
    if (a2) argv[argc++] = dup_cstr(a2);
    if (a3) argv[argc++] = dup_cstr(a3);
    if (a4) argv[argc++] = dup_cstr(a4);
    if (a5) argv[argc++] = dup_cstr(a5);
    if (a6) argv[argc++] = dup_cstr(a6);
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

void test_cli_parse_version_command(void) {
    int argc;
    char **argv = make_argv("cortex", "--version", NULL, NULL, &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_VERSION);
    ASSERT_TRUE(parsed.name == NULL);

    free_argv(argv, argc);
}

void test_cli_parse_generate_controller_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "controller", "users", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_CONTROLLER);
    ASSERT_STR_EQ(parsed.name, "users");

    free_argv(argv, argc);
}

void test_cli_parse_generate_resource_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "resource", "posts", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_RESOURCE);
    ASSERT_STR_EQ(parsed.name, "posts");

    free_argv(argv, argc);
}

void test_cli_parse_generate_model_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "model", "post", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_MODEL);
    ASSERT_STR_EQ(parsed.name, "post");

    free_argv(argv, argc);
}

void test_cli_parse_generate_service_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "service", "mailer", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_SERVICE);
    ASSERT_STR_EQ(parsed.name, "mailer");

    free_argv(argv, argc);
}

void test_cli_parse_generate_neural_model_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "neural:model", "incident", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_NEURAL_MODEL);
    ASSERT_STR_EQ(parsed.name, "incident");

    free_argv(argv, argc);
}

void test_cli_parse_generate_neural_prompt_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "neural:prompt", "incident", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_NEURAL_PROMPT);
    ASSERT_STR_EQ(parsed.name, "incident");

    free_argv(argv, argc);
}

void test_cli_parse_generate_neural_agent_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "neural:agent", "incident", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_NEURAL_AGENT);
    ASSERT_STR_EQ(parsed.name, "incident");

    free_argv(argv, argc);
}

void test_cli_parse_generate_neural_rag_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "neural:rag", "incident", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_NEURAL_RAG);
    ASSERT_STR_EQ(parsed.name, "incident");

    free_argv(argv, argc);
}

void test_cli_parse_generate_neural_stream_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "neural:stream", "incident", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_NEURAL_STREAM);
    ASSERT_STR_EQ(parsed.name, "incident");

    free_argv(argv, argc);
}

void test_cli_parse_generate_neural_memory_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "neural:memory", "incident", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_NEURAL_MEMORY);
    ASSERT_STR_EQ(parsed.name, "incident");

    free_argv(argv, argc);
}

void test_cli_parse_generate_neural_retriever_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "neural:retriever", "incident", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_NEURAL_RETRIEVER);
    ASSERT_STR_EQ(parsed.name, "incident");

    free_argv(argv, argc);
}

void test_cli_parse_generate_neural_integration_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "neural:integration", "openai", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_NEURAL_INTEGRATION);
    ASSERT_STR_EQ(parsed.name, "openai");

    free_argv(argv, argc);
}

void test_cli_parse_generate_neural_policy_command(void) {
    int argc;
    char **argv = make_argv("cortex", "generate", "neural:policy", "incident", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_NEURAL_POLICY);
    ASSERT_STR_EQ(parsed.name, "incident");

    free_argv(argv, argc);
}

void test_cli_parse_db_migrate_command(void) {
    int argc;
    char **argv = make_argv("cortex", "db:migrate", NULL, NULL, &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_DB_MIGRATE);
    ASSERT_TRUE(parsed.name == NULL);

    free_argv(argv, argc);
}

void test_cli_parse_db_create_command(void) {
    int argc;
    char **argv = make_argv("cortex", "db:create", NULL, NULL, &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_DB_CREATE);
    ASSERT_TRUE(parsed.name == NULL);

    free_argv(argv, argc);
}

void test_cli_parse_generate_stimulus_command(void) {
    int argc;
    char **argv = make_argv6("cortex", "generate", "stimulus", "post", NULL, NULL, &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_STIMULUS);
    ASSERT_STR_EQ(parsed.name, "post");

    free_argv(argv, argc);
}

void test_cli_parse_generate_scaffold_react_flags(void) {
    int argc;
    char **argv = make_argv7("cortex", "generate", "scaffold", "Post", "title:string", "body:text", "--no-react", &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_GENERATE_SCAFFOLD);
    ASSERT_STR_EQ(parsed.name, "Post");
    ASSERT_EQ(parsed.attribute_count, 2);
    ASSERT_EQ(parsed.use_react, 0);

    free_argv(argv, argc);
}

void test_cli_parse_new_command(void) {
    int argc;
    char **argv = make_argv("cortex", "new", "myapp", NULL, &argc);
    CliParsed parsed;

    ASSERT_EQ(cli_parse(argc, argv, &parsed), 0);
    ASSERT_EQ(parsed.command, CLI_COMMAND_NEW);
    ASSERT_STR_EQ(parsed.name, "myapp");

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

static int dir_exists(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        return 0;
    }
    return S_ISDIR(st.st_mode) ? 1 : 0;
}

static int file_contains(const char *path, const char *substring) {
    FILE *f;
    char buf[1024];
    size_t sublen;
    int found = 0;

    f = fopen(path, "r");
    if (f == NULL) return 0;
    sublen = strlen(substring);
    if (sublen == 0) { fclose(f); return 1; }
    while (fgets(buf, (int)sizeof(buf), f) != NULL && !found) {
        if (strstr(buf, substring) != NULL) found = 1;
    }
    fclose(f);
    return found;
}

static char *read_file_alloc(const char *path) {
    FILE *f;
    long sz;
    size_t nread;
    char *buf;

    f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return NULL;
    }
    sz = ftell(f);
    if (sz < 0) {
        fclose(f);
        return NULL;
    }
    if (fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    buf = (char *)malloc((size_t)sz + 1);
    if (!buf) {
        fclose(f);
        return NULL;
    }
    nread = fread(buf, 1, (size_t)sz, f);
    fclose(f);
    if (nread != (size_t)sz) {
        free(buf);
        return NULL;
    }
    buf[sz] = '\0';
    return buf;
}

static int write_file_from_string(const char *path, const char *content) {
    FILE *f;
    size_t len;
    size_t nwritten;

    if (!path || !content) return -1;
    f = fopen(path, "wb");
    if (!f) return -1;
    len = strlen(content);
    nwritten = fwrite(content, 1, len, f);
    if (fclose(f) != 0) return -1;
    if (nwritten != len) return -1;
    return 0;
}

/* Remove generated project dir and contents so tests don't leave artifacts. */
static void remove_project_dir(const char *project_name) {
    char cmd[512];
    int i;

    if (!project_name || project_name[0] == '\0') {
        return;
    }

    for (i = 0; project_name[i] != '\0'; ++i) {
        char c = project_name[i];
        if (!((c >= 'a' && c <= 'z') ||
              (c >= 'A' && c <= 'Z') ||
              (c >= '0' && c <= '9') ||
              c == '_' || c == '-')) {
            return;
        }
    }

    if (snprintf(cmd, sizeof(cmd), "rm -rf -- %s", project_name) < 0) {
        return;
    }
    (void)system(cmd);
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

void test_cli_dispatch_generate_resource_executes_handler(void) {
    const char *controller_path = "app/controllers/specclis_controller.c";
    const char *index_view_path = "app/views/specclis/index.html";
    const char *show_view_path = "app/views/specclis/show.html";
    const char *new_view_path = "app/views/specclis/new.html";
    const char *edit_view_path = "app/views/specclis/edit.html";
    const char *routes_path = "config/routes.c";
    CliParsed parsed;
    int had_routes_file = file_exists(routes_path);
    char *routes_backup = NULL;

    if (had_routes_file) {
        routes_backup = read_file_alloc(routes_path);
        ASSERT_TRUE(routes_backup != NULL);
    }

    remove_if_exists(controller_path);
    remove_if_exists(index_view_path);
    remove_if_exists(show_view_path);
    remove_if_exists(new_view_path);
    remove_if_exists(edit_view_path);
    if (!had_routes_file) {
        remove_if_exists(routes_path);
    }

    parsed.command = CLI_COMMAND_GENERATE_RESOURCE;
    parsed.name = "speccli";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(controller_path));
    ASSERT_TRUE(file_exists(index_view_path));

    remove_if_exists(controller_path);
    remove_if_exists(index_view_path);
    remove_if_exists(show_view_path);
    remove_if_exists(new_view_path);
    remove_if_exists(edit_view_path);
    if (had_routes_file && routes_backup) {
        ASSERT_EQ(write_file_from_string(routes_path, routes_backup), 0);
    } else {
        remove_if_exists(routes_path);
    }
    free(routes_backup);
}

void test_cli_dispatch_generate_model_executes_handler(void) {
    const char *path = "app/models/post.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_MODEL;
    parsed.name = "post";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_cli_dispatch_generate_service_executes_handler(void) {
    const char *path = "service/mailer.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_SERVICE;
    parsed.name = "mailer";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_cli_dispatch_generate_neural_model_executes_handler(void) {
    const char *path = "app/neural/incident_neural_model.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_NEURAL_MODEL;
    parsed.name = "incident";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_cli_dispatch_generate_neural_prompt_executes_handler(void) {
    const char *path = "app/neural/prompts/incident_prompt.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_NEURAL_PROMPT;
    parsed.name = "incident";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_cli_dispatch_generate_neural_agent_executes_handler(void) {
    const char *path = "app/neural/agents/incident_agent.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_NEURAL_AGENT;
    parsed.name = "incident";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_cli_dispatch_generate_neural_rag_executes_handler(void) {
    const char *path = "app/neural/rag/incident_rag.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_NEURAL_RAG;
    parsed.name = "incident";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_cli_dispatch_generate_neural_stream_executes_handler(void) {
    const char *path = "app/neural/streams/incident_stream.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_NEURAL_STREAM;
    parsed.name = "incident";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_cli_dispatch_generate_neural_memory_executes_handler(void) {
    const char *path = "app/neural/memory/incident_memory.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_NEURAL_MEMORY;
    parsed.name = "incident";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_cli_dispatch_generate_neural_retriever_executes_handler(void) {
    const char *path = "app/neural/retrievers/incident_retriever.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_NEURAL_RETRIEVER;
    parsed.name = "incident";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_cli_dispatch_generate_neural_integration_executes_handler(void) {
    const char *path = "app/neural/integrations/openai_integration.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_NEURAL_INTEGRATION;
    parsed.name = "openai";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_cli_dispatch_generate_neural_policy_executes_handler(void) {
    const char *path = "app/neural/policies/incident_policy.c";
    CliParsed parsed;

    remove_if_exists(path);

    parsed.command = CLI_COMMAND_GENERATE_NEURAL_POLICY;
    parsed.name = "incident";

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(file_exists(path));

    remove_if_exists(path);
}

void test_cli_dispatch_db_migrate_executes_handler(void) {
    CliParsed parsed;
    parsed.command = CLI_COMMAND_DB_MIGRATE;
    parsed.name = NULL;

    ASSERT_EQ(cli_dispatch(&parsed), 0);
}

void test_cli_dispatch_db_create_executes_handler(void) {
    const char *storage = "db/development.sqlite3";
    const char *dbdir = "db";
    CliParsed parsed;

    remove_if_exists(storage);
    if (dir_exists(dbdir)) {
        /* Best effort: remove empty directory if possible. */
        (void)rmdir(dbdir);
    }

    parsed.command = CLI_COMMAND_DB_CREATE;
    parsed.name = NULL;

    ASSERT_EQ(cli_dispatch(&parsed), 0);
    ASSERT_TRUE(dir_exists(dbdir));
    ASSERT_TRUE(file_exists(storage));

    remove_if_exists(storage);
    if (dir_exists(dbdir)) {
        (void)rmdir(dbdir);
    }
}

/* cortex new: verify project directory and folder structure (app/, config/, db/). */
void test_forge_new_creates_project_directory(void) {
    const char *project_name = "cortex_new_test_project";
    char path[256];

    remove_project_dir(project_name);

    ASSERT_EQ(forge_new_project(project_name), 0);

    snprintf(path, sizeof(path), "%s/main.c", project_name);
    ASSERT_TRUE(file_exists(path));
    snprintf(path, sizeof(path), "%s/package.json", project_name);
    ASSERT_TRUE(file_exists(path));
    snprintf(path, sizeof(path), "%s/app/javascript/application.jsx", project_name);
    ASSERT_TRUE(file_exists(path));
    snprintf(path, sizeof(path), "%s/app", project_name);
    ASSERT_TRUE(dir_exists(path));
    snprintf(path, sizeof(path), "%s/config", project_name);
    ASSERT_TRUE(dir_exists(path));
    snprintf(path, sizeof(path), "%s/db", project_name);
    ASSERT_TRUE(dir_exists(path));
    snprintf(path, sizeof(path), "%s/db/development.sqlite3", project_name);
    ASSERT_TRUE(file_exists(path));

    remove_project_dir(project_name);
}

/* cortex new: verify basic files main.c and Makefile exist. */
void test_forge_new_creates_main_c_and_makefile(void) {
    const char *project_name = "cortex_new_test_files";
    char path[256];

    remove_project_dir(project_name);

    ASSERT_EQ(forge_new_project(project_name), 0);
    snprintf(path, sizeof(path), "%s/main.c", project_name);
    ASSERT_TRUE(file_exists(path));
    snprintf(path, sizeof(path), "%s/Makefile", project_name);
    ASSERT_TRUE(file_exists(path));
    ASSERT_TRUE(file_contains(path, "server: cortex_app assets-build"));
    ASSERT_TRUE(file_contains(path, "$(CORTEX_ROOT)/cortex assets:build"));
    ASSERT_TRUE(file_contains(path, "$(CORTEX_ROOT)/cortex dev"));
    snprintf(path, sizeof(path), "%s/package.json", project_name);
    ASSERT_TRUE(file_exists(path));
    snprintf(path, sizeof(path), "%s/app/javascript/application.jsx", project_name);
    ASSERT_TRUE(file_exists(path));

    remove_project_dir(project_name);
}

/* cortex new via CLI: dispatch creates project and all expected paths exist. */
void test_cli_dispatch_new_creates_project(void) {
    const char *project_name = "cortex_new_cli_test";
    CliParsed parsed;
    char path[256];

    remove_project_dir(project_name);

    parsed.command = CLI_COMMAND_NEW;
    parsed.name = project_name;
    ASSERT_EQ(cli_dispatch(&parsed), 0);

    snprintf(path, sizeof(path), "%s/main.c", project_name);
    ASSERT_TRUE(file_exists(path));
    snprintf(path, sizeof(path), "%s/Makefile", project_name);
    ASSERT_TRUE(file_exists(path));
    snprintf(path, sizeof(path), "%s/package.json", project_name);
    ASSERT_TRUE(file_exists(path));
    snprintf(path, sizeof(path), "%s/app/javascript/application.jsx", project_name);
    ASSERT_TRUE(file_exists(path));
    snprintf(path, sizeof(path), "%s/app", project_name);
    ASSERT_TRUE(dir_exists(path));
    snprintf(path, sizeof(path), "%s/config", project_name);
    ASSERT_TRUE(dir_exists(path));
    snprintf(path, sizeof(path), "%s/db", project_name);
    ASSERT_TRUE(dir_exists(path));
    snprintf(path, sizeof(path), "%s/db/development.sqlite3", project_name);
    ASSERT_TRUE(file_exists(path));

    remove_project_dir(project_name);
}

