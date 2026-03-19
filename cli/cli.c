#include "cli.h"

#include <stdio.h>
#include <string.h>

#include "../forge/forge_generators.h"
#include "../core/active_migration.h"
#include "../db/db_create.h"
#include "../db/db_migrate.h"
#include "../action/action_dispatch.h"
#include "../action/action_controller.h"
#include "../action/action_router.h"

int cli_parse(int argc, char **argv, CliParsed *out) {
    if (!out || argc < 2) {
        return -1;
    }

    out->command = CLI_COMMAND_NONE;
    out->name = NULL;

    /* argv[0] is the program name, argv[1] is the first word. */
    if (strcmp(argv[1], "server") == 0) {
        if (argc != 2) {
            return -1;
        }
        out->command = CLI_COMMAND_SERVER;
        return 0;
    }

    if (strcmp(argv[1], "new") == 0) {
        if (argc != 3) {
            return -1;
        }
        out->command = CLI_COMMAND_NEW;
        out->name = argv[2];
        return 0;
    }

    if (strcmp(argv[1], "generate") == 0) {
        if (argc < 3) {
            return -1;
        }
        if (strcmp(argv[2], "controller") == 0) {
            if (argc != 4) {
                return -1;
            }
            out->command = CLI_COMMAND_GENERATE_CONTROLLER;
            out->name = argv[3];
            return 0;
        }
        return -1;
    }

    if (strcmp(argv[1], "db:migrate") == 0) {
        if (argc != 2) {
            return -1;
        }
        out->command = CLI_COMMAND_DB_MIGRATE;
        return 0;
    }

    /* Allow the alternate form: `cortex db migrate`. */
    if (strcmp(argv[1], "db") == 0) {
        if (argc != 3) {
            return -1;
        }
        if (strcmp(argv[2], "migrate") == 0) {
            out->command = CLI_COMMAND_DB_MIGRATE;
            return 0;
        }
    }

    if (strcmp(argv[1], "db:create") == 0) {
        if (argc != 2) {
            return -1;
        }
        out->command = CLI_COMMAND_DB_CREATE;
        return 0;
    }

    /* Allow the alternate form: `cortex db create`. */
    if (strcmp(argv[1], "db") == 0) {
        if (argc != 3) {
            return -1;
        }
        if (strcmp(argv[2], "create") != 0) {
            return -1;
        }
        out->command = CLI_COMMAND_DB_CREATE;
        return 0;
    }

    return -1;
}

static int cli_handle_server(void) {
    ActionRouter router;

    action_router_init(&router);

    /* Minimal health endpoint so the server does something useful. */
    void health_controller(ActionRequest *req, ActionResponse *res) {
        (void)req;
        action_controller_render_text(res, 200, "OK");
    }

    if (action_router_add_route(&router, "GET", "/health", health_controller) != 0) {
        fprintf(stderr, "failed to register /health route\n");
        return -1;
    }

    return action_dispatch_serve_http(&router);
}

static int cli_handle_new(const char *project_name) {
    int rc;
    if (!project_name || project_name[0] == '\0') {
        return -1;
    }
    rc = forge_new_project(project_name);
    if (rc != 0) {
        fprintf(stderr, "failed to create project %s\n", project_name);
        return rc;
    }
    return 0;
}

static int cli_handle_generate_controller(const char *name) {
    int rc;
    if (!name || name[0] == '\0') {
        return -1;
    }
    rc = forge_generate_controller(name);
    if (rc != 0) {
        fprintf(stderr, "failed to generate controller for %s\n", name);
        return rc;
    }
    return 0;
}

static int cli_handle_db_migrate(void) {
    return db_migrate_default("db/storage.json");
}

static int cli_handle_db_create(void) {
    /* Default storage is `db/storage.json`. */
    return db_create("db/storage.json");
}

int cli_dispatch(const CliParsed *parsed) {
    if (!parsed) {
        return -1;
    }

    switch (parsed->command) {
    case CLI_COMMAND_SERVER:
        return cli_handle_server();
    case CLI_COMMAND_NEW:
        return cli_handle_new(parsed->name);
    case CLI_COMMAND_GENERATE_CONTROLLER:
        return cli_handle_generate_controller(parsed->name);
    case CLI_COMMAND_DB_MIGRATE:
        return cli_handle_db_migrate();
    case CLI_COMMAND_DB_CREATE:
        return cli_handle_db_create();
    default:
        return -1;
    }
}

int cli_run(int argc, char **argv) {
    CliParsed parsed;
    if (cli_parse(argc, argv, &parsed) != 0) {
        fprintf(stderr, "Usage:\n");
        fprintf(stderr, "  %s server\n", argv[0]);
        fprintf(stderr, "  %s new <project_name>\n", argv[0]);
        fprintf(stderr, "  %s generate controller <name>\n", argv[0]);
        fprintf(stderr, "  %s db:migrate\n", argv[0]);
        fprintf(stderr, "  %s db:create\n", argv[0]);
        return -1;
    }
    return cli_dispatch(&parsed);
}

