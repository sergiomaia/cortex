#ifndef ACTIVE_ACTIVE_MIGRATION_H
#define ACTIVE_ACTIVE_MIGRATION_H

/*
 * ActiveRecord-style migration helpers + portable SQL fragments.
 * Prefer this header in app migration code; it includes the core registry API.
 */

#include "../core/active_migration.h"

#ifdef CORTEX_HAVE_POSTGRES
#define SQL_PK       "SERIAL PRIMARY KEY"
#define SQL_NOW      "NOW()"
#define SQL_BOOL     "BOOLEAN"
#define SQL_DATETIME "TIMESTAMPTZ"
#else
#define SQL_PK       "INTEGER PRIMARY KEY AUTOINCREMENT"
#define SQL_NOW      "CURRENT_TIMESTAMP"
#define SQL_BOOL     "INTEGER"
#define SQL_DATETIME "TEXT"
#endif

#endif /* ACTIVE_ACTIVE_MIGRATION_H */
