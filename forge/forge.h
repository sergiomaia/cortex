#ifndef FORGE_H
#define FORGE_H

/* Database CLI helpers used by the cortex binary (see cli/cli.c). */

int forge_db_migrate(void);
int forge_db_rollback(int steps);
int forge_db_status(void);
int forge_generate_migration(const char *name);

#endif /* FORGE_H */
