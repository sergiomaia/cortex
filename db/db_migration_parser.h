#ifndef DB_MIGRATION_PARSER_H
#define DB_MIGRATION_PARSER_H

#include <stddef.h>

/*
 * Parsed body of a db/migrate SQL file using section markers
 * migrate:up and migrate:down.
 * section markers. Text outside those sections is ignored.
 */
typedef struct {
    char *up_sql;
    char *down_sql; /* NULL when the file has no down section */
} MigrationSQL;

void migration_sql_init(MigrationSQL *out);

/* Frees non-NULL pointers and zeroes the struct. */
void migration_sql_free(MigrationSQL *m);

/*
 * Reads filepath and fills *out. Caller must call migration_sql_free even on failure
 * after a successful partial parse (returns -1 only after init or full cleanup).
 *
 * Returns 0 on success, -1 on error (missing file, malformed file, allocation failure).
 */
int migration_sql_parse(const char *filepath, MigrationSQL *out);

#endif /* DB_MIGRATION_PARSER_H */
