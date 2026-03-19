#ifndef CLI_H
#define CLI_H

/* Very small CLI for the cortex binary.
 *
 * Supported commands:
 *   cortex server
 *   cortex new <project_name>
 *   cortex generate controller <name>
 *   cortex db:migrate
 *   cortex db:create
 *
 * The public API is kept tiny so tests can exercise parsing and
 * dispatching without spawning a real server process.
 */

typedef enum {
    CLI_COMMAND_NONE = 0,
    CLI_COMMAND_VERSION,
    CLI_COMMAND_SERVER,
    CLI_COMMAND_NEW,
    CLI_COMMAND_GENERATE_CONTROLLER,
    CLI_COMMAND_GENERATE_SCAFFOLD,
    CLI_COMMAND_DB_MIGRATE,
    CLI_COMMAND_DB_CREATE
} CliCommand;

typedef struct {
    CliCommand command;
    const char *name; /* Used by new and generate controller, NULL otherwise. */
    const char **attributes; /* Used by generate scaffold. */
    int attribute_count; /* Number of entries in attributes. */
} CliParsed;

/* Parse argv (including program name at argv[0]) into a CliParsed value.
 * Returns 0 on success, non‑zero on error.
 */
int cli_parse(int argc, char **argv, CliParsed *out);

/* Dispatch a parsed command.
 * Returns 0 on success, non‑zero on error.
 *
 * For CLI_COMMAND_SERVER this will start the HTTP server and
 * block until the server loop exits.
 */
int cli_dispatch(const CliParsed *parsed);

/* Convenience entry that combines parse + dispatch. */
int cli_run(int argc, char **argv);

#endif /* CLI_H */

