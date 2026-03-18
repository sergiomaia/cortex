#ifndef CLI_H
#define CLI_H

/* Very small CLI for the cortex binary.
 *
 * Supported commands:
 *   cortex server
 *   cortex generate controller <name>
 *
 * The public API is kept tiny so tests can exercise parsing and
 * dispatching without spawning a real server process.
 */

typedef enum {
    CLI_COMMAND_NONE = 0,
    CLI_COMMAND_SERVER,
    CLI_COMMAND_GENERATE_CONTROLLER
} CliCommand;

typedef struct {
    CliCommand command;
    const char *name; /* Used by generate controller, NULL otherwise. */
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

