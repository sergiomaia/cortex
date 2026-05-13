#include "db_migration_schema.h"

#include "../core/pulse.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef CORTEX_HAVE_POSTGRES
#include "db_connection.h"
#endif

static int sqlite_table_exists(DbConnection *conn, const char *table) {
    DbStatement *st = NULL;
    int step;
    char sql[256];

    if (!conn || !table) {
        return -1;
    }
    if (snprintf(sql, sizeof(sql),
                 "SELECT 1 FROM sqlite_master WHERE type='table' AND name='%s'",
                 table) >= (int)sizeof(sql)) {
        return -1;
    }
    if (db_connection_prepare(conn, sql, &st) != 0) {
        return -1;
    }
    step = db_statement_step(st);
    db_statement_finalize(st);
    if (step < 0) {
        return -1;
    }
    return step == 1 ? 1 : 0;
}

static int sqlite_column_count(DbConnection *conn, const char *table, int *out_count) {
    DbStatement *st = NULL;
    int step;
    int n = 0;
    char pragma_sql[256];

    if (!conn || !table || !out_count) {
        return -1;
    }
    if (snprintf(pragma_sql, sizeof(pragma_sql), "PRAGMA table_info(%s)", table) >= (int)sizeof(pragma_sql)) {
        return -1;
    }
    if (db_connection_prepare(conn, pragma_sql, &st) != 0) {
        return -1;
    }
    while (1) {
        step = db_statement_step(st);
        if (step == 0) {
            break;
        }
        if (step < 0) {
            db_statement_finalize(st);
            return -1;
        }
        n++;
    }
    db_statement_finalize(st);
    *out_count = n;
    return 0;
}

static int sqlite_migrate_legacy_to_unified(DbConnection *conn) {
    /*
     * Rebuild schema_migrations from legacy INTEGER-only rows and optional
     * cortex_sql_migrations (filename keys) into unified TEXT version rows.
     */
    if (db_connection_exec(conn, "DROP TABLE IF EXISTS schema_migrations__cortex_rebuild;") != 0) {
        return -1;
    }
    if (db_connection_exec(conn, "BEGIN IMMEDIATE;") != 0) {
        return -1;
    }
    if (db_connection_exec(conn,
                           "CREATE TABLE IF NOT EXISTS schema_migrations__cortex_rebuild (\n"
                           "  version TEXT PRIMARY KEY NOT NULL,\n"
                           "  name TEXT NOT NULL,\n"
                           "  applied_at TEXT DEFAULT CURRENT_TIMESTAMP\n"
                           ");") != 0) {
        (void)db_connection_exec(conn, "ROLLBACK;");
        return -1;
    }

    if (db_connection_exec(conn,
                           "INSERT OR IGNORE INTO schema_migrations__cortex_rebuild (version, name, applied_at)\n"
                           "SELECT printf('%014d', version), '', CURRENT_TIMESTAMP FROM schema_migrations;") != 0) {
        (void)db_connection_exec(conn, "ROLLBACK;");
        return -1;
    }

    if (sqlite_table_exists(conn, "cortex_sql_migrations") == 1) {
        if (db_connection_exec(conn,
                               "INSERT OR IGNORE INTO schema_migrations__cortex_rebuild (version, name, applied_at)\n"
                               "SELECT substr(name, 1, 14),\n"
                               "       REPLACE(substr(name, instr(name, '_') + 1), '.sql', ''),\n"
                               "       CURRENT_TIMESTAMP\n"
                               "FROM cortex_sql_migrations\n"
                               "WHERE name GLOB '[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]_*.sql';") != 0) {
            (void)db_connection_exec(conn, "ROLLBACK;");
            return -1;
        }
    }

    if (db_connection_exec(conn, "DROP TABLE schema_migrations;") != 0) {
        (void)db_connection_exec(conn, "ROLLBACK;");
        return -1;
    }
    if (db_connection_exec(conn, "ALTER TABLE schema_migrations__cortex_rebuild RENAME TO schema_migrations;") != 0) {
        (void)db_connection_exec(conn, "ROLLBACK;");
        return -1;
    }
    if (db_connection_exec(conn, "COMMIT;") != 0) {
        return -1;
    }
    return 0;
}

static int sqlite_ensure_unified_schema(DbConnection *conn) {
    int exists;
    int col_count = 0;

    exists = sqlite_table_exists(conn, "schema_migrations");
    if (exists < 0) {
        return -1;
    }
    if (!exists) {
        if (db_connection_exec(conn,
                               "CREATE TABLE schema_migrations (\n"
                               "  version TEXT PRIMARY KEY NOT NULL,\n"
                               "  name TEXT NOT NULL,\n"
                               "  applied_at TEXT DEFAULT CURRENT_TIMESTAMP\n"
                               ");") != 0) {
            return -1;
        }
        LOG_INFO("db.migration.schema", "created unified schema_migrations table (SQLite)");
        return 0;
    }

    if (sqlite_column_count(conn, "schema_migrations", &col_count) != 0) {
        return -1;
    }
    if (col_count == 1) {
        LOG_WARN("db.migration.schema", "migrating legacy schema_migrations (INTEGER) to unified TEXT layout");
        if (sqlite_migrate_legacy_to_unified(conn) != 0) {
            return -1;
        }
        return 0;
    }
    if (col_count < 3) {
        /* Add missing columns for older experiments. */
        if (col_count == 2) {
            if (db_connection_exec(conn, "ALTER TABLE schema_migrations ADD COLUMN applied_at TEXT DEFAULT CURRENT_TIMESTAMP;") != 0) {
                return -1;
            }
        }
    }
    return 0;
}

#ifdef CORTEX_HAVE_POSTGRES
static int pg_table_exists(DbConnection *conn, const char *table) {
    DbStatement *st = NULL;
    int step;

    if (!conn || !table) {
        return -1;
    }
    if (db_connection_prepare(conn,
                               "SELECT 1 FROM information_schema.tables "
                               "WHERE table_schema = 'public' AND table_name = ?",
                               &st) != 0) {
        return -1;
    }
    if (db_statement_bind_text(st, 1, table) != 0) {
        db_statement_finalize(st);
        return -1;
    }
    step = db_statement_step(st);
    db_statement_finalize(st);
    if (step < 0) {
        return -1;
    }
    return step == 1 ? 1 : 0;
}

static int pg_migrate_legacy_to_unified(DbConnection *conn) {
    if (db_connection_exec(conn,
                           "CREATE TABLE schema_migrations__cortex_rebuild (\n"
                           "  version VARCHAR(14) PRIMARY KEY NOT NULL,\n"
                           "  name VARCHAR(255) NOT NULL,\n"
                           "  applied_at TIMESTAMPTZ DEFAULT NOW()\n"
                           ");") != 0) {
        return -1;
    }
    if (db_connection_exec(conn,
                           "INSERT INTO schema_migrations__cortex_rebuild (version, name, applied_at)\n"
                           "SELECT LPAD(version::text, 14, '0'), '', NOW() FROM schema_migrations\n"
                           "ON CONFLICT DO NOTHING;") != 0) {
        return -1;
    }
    if (pg_table_exists(conn, "cortex_sql_migrations") == 1) {
        if (db_connection_exec(conn,
                               "INSERT INTO schema_migrations__cortex_rebuild (version, name, applied_at)\n"
                               "SELECT substring(name from 1 for 14),\n"
                               "       CASE WHEN length(name) > 15 THEN substring(name from 16) ELSE '' END,\n"
                               "       NOW()\n"
                               "FROM cortex_sql_migrations\n"
                               "WHERE name ~ '^[0-9]{14}_.*\\.sql$'\n"
                               "ON CONFLICT DO NOTHING;") != 0) {
            return -1;
        }
    }
    if (db_connection_exec(conn, "DROP TABLE schema_migrations;") != 0) {
        return -1;
    }
    if (db_connection_exec(conn, "ALTER TABLE schema_migrations__cortex_rebuild RENAME TO schema_migrations;") != 0) {
        return -1;
    }
    return 0;
}

static int pg_column_count(DbConnection *conn, const char *table, int *out_count) {
    DbStatement *st = NULL;
    int step;
    int n = 0;

    if (!conn || !table || !out_count) {
        return -1;
    }
    if (db_connection_prepare(conn,
                               "SELECT COUNT(*) FROM information_schema.columns "
                               "WHERE table_schema='public' AND table_name=?",
                               &st) != 0) {
        return -1;
    }
    if (db_statement_bind_text(st, 1, table) != 0) {
        db_statement_finalize(st);
        return -1;
    }
    step = db_statement_step(st);
    if (step != 1) {
        db_statement_finalize(st);
        return -1;
    }
    n = db_statement_column_int(st, 0);
    db_statement_finalize(st);
    *out_count = n;
    return 0;
}

static int pg_ensure_unified_schema(DbConnection *conn) {
    int exists;
    int col_count = 0;

    exists = pg_table_exists(conn, "schema_migrations");
    if (exists < 0) {
        return -1;
    }
    if (!exists) {
        if (db_connection_exec(conn,
                               "CREATE TABLE schema_migrations (\n"
                               "  version VARCHAR(14) PRIMARY KEY NOT NULL,\n"
                               "  name VARCHAR(255) NOT NULL,\n"
                               "  applied_at TIMESTAMPTZ DEFAULT NOW()\n"
                               ");") != 0) {
            return -1;
        }
        LOG_INFO("db.migration.schema", "created unified schema_migrations table (PostgreSQL)");
        return 0;
    }
    if (pg_column_count(conn, "schema_migrations", &col_count) != 0) {
        return -1;
    }
    if (col_count == 1) {
        LOG_WARN("db.migration.schema", "migrating legacy PostgreSQL schema_migrations to unified layout");
        if (pg_migrate_legacy_to_unified(conn) != 0) {
            return -1;
        }
        return 0;
    }
    return 0;
}
#endif

int db_migration_ensure_table(DbConnection *conn) {
    if (!conn) {
        return -1;
    }
#ifdef CORTEX_HAVE_POSTGRES
    if (db_connection_backend(conn) == CORTEX_DB_BACKEND_POSTGRESQL) {
        return pg_ensure_unified_schema(conn);
    }
#endif
    return sqlite_ensure_unified_schema(conn);
}

int db_migration_format_callback_version(int version, char *out, size_t out_size) {
    if (version <= 0 || !out || out_size < 15) {
        return -1;
    }
    if (snprintf(out, out_size, "%014d", version) >= (int)out_size) {
        return -1;
    }
    if (strlen(out) != 14) {
        return -1;
    }
    /* Timestamps sort after callback rows when using lexicographic order below 19900101000000 sentinel. */
    if (strcmp(out, "19900101000000") >= 0) {
        LOG_ERROR("db.migration.schema", "callback migration version out of supported range");
        return -1;
    }
    return 0;
}

int db_migration_is_applied(DbConnection *conn, const char *version, int *out_applied) {
    DbStatement *st = NULL;
    int step;

    if (!conn || !version || !out_applied) {
        return -1;
    }
    *out_applied = 0;
    if (db_connection_prepare(conn, "SELECT 1 FROM schema_migrations WHERE version = ?", &st) != 0) {
        return -1;
    }
    if (db_statement_bind_text(st, 1, version) != 0) {
        db_statement_finalize(st);
        return -1;
    }
    step = db_statement_step(st);
    if (step < 0) {
        db_statement_finalize(st);
        return -1;
    }
    db_statement_finalize(st);
    *out_applied = step == 1 ? 1 : 0;
    return 0;
}

int db_migration_mark_applied(DbConnection *conn, const char *version, const char *name) {
    DbStatement *st = NULL;
    int step;
    const char *nm = name ? name : "";

    if (!conn || !version) {
        return -1;
    }
    if (db_connection_prepare(conn, "INSERT INTO schema_migrations (version, name) VALUES (?, ?)", &st) != 0) {
        return -1;
    }
    if (db_statement_bind_text(st, 1, version) != 0 || db_statement_bind_text(st, 2, nm) != 0) {
        db_statement_finalize(st);
        return -1;
    }
    step = db_statement_step(st);
    db_statement_finalize(st);
    if (step != 0) {
        return -1;
    }
    return 0;
}

int db_migration_mark_rolled_back(DbConnection *conn, const char *version) {
    DbStatement *st = NULL;
    int step;

    if (!conn || !version) {
        return -1;
    }
    if (db_connection_prepare(conn, "DELETE FROM schema_migrations WHERE version = ?", &st) != 0) {
        return -1;
    }
    if (db_statement_bind_text(st, 1, version) != 0) {
        db_statement_finalize(st);
        return -1;
    }
    step = db_statement_step(st);
    db_statement_finalize(st);
    if (step != 0) {
        return -1;
    }
    return 0;
}
