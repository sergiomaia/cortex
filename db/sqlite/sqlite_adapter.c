#include "../db_connection.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

struct DbConnection {
    sqlite3 *handle;
};

struct DbStatement {
    sqlite3_stmt *stmt;
};

int db_connection_open(const char *path, DbConnection **out) {
    sqlite3 *db;
    int rc;

    if (!path || path[0] == '\0' || !out) {
        return -1;
    }

    *out = NULL;
    rc = sqlite3_open(path, &db);
    if (rc != SQLITE_OK) {
        if (db) {
            fprintf(stderr, "db: sqlite3_open failed for '%s': %s\n", path, sqlite3_errmsg(db));
            sqlite3_close(db);
        } else {
            fprintf(stderr, "db: sqlite3_open failed for '%s'\n", path);
        }
        return -1;
    }

    *out = (DbConnection *)calloc(1, sizeof(DbConnection));
    if (!*out) {
        sqlite3_close(db);
        fprintf(stderr, "db: out of memory opening database\n");
        return -1;
    }

    (*out)->handle = db;
    return 0;
}

void db_connection_close(DbConnection *conn) {
    if (!conn) {
        return;
    }
    if (conn->handle) {
        sqlite3_close(conn->handle);
        conn->handle = NULL;
    }
    free(conn);
}

int db_connection_exec(DbConnection *conn, const char *sql) {
    char *errmsg = NULL;
    int rc;

    if (!conn || !conn->handle || !sql) {
        return -1;
    }

    rc = sqlite3_exec(conn->handle, sql, NULL, NULL, &errmsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "db: SQL error: %s\n", errmsg ? errmsg : sqlite3_errmsg(conn->handle));
        if (errmsg) {
            sqlite3_free(errmsg);
        }
        return -1;
    }
    return 0;
}

int db_connection_prepare(DbConnection *conn, const char *sql, DbStatement **out) {
    sqlite3_stmt *st = NULL;
    int rc;

    if (!conn || !conn->handle || !sql || !out) {
        return -1;
    }

    *out = NULL;
    rc = sqlite3_prepare_v2(conn->handle, sql, -1, &st, NULL);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "db: prepare failed: %s\n", sqlite3_errmsg(conn->handle));
        return -1;
    }

    *out = (DbStatement *)calloc(1, sizeof(DbStatement));
    if (!*out) {
        sqlite3_finalize(st);
        fprintf(stderr, "db: out of memory preparing statement\n");
        return -1;
    }
    (*out)->stmt = st;
    return 0;
}

int db_statement_step(DbStatement *stmt) {
    int rc;

    if (!stmt || !stmt->stmt) {
        return -1;
    }

    rc = sqlite3_step(stmt->stmt);
    if (rc == SQLITE_ROW) {
        return 1;
    }
    if (rc == SQLITE_DONE) {
        return 0;
    }
    fprintf(stderr, "db: step failed: %s\n", sqlite3_errmsg(sqlite3_db_handle(stmt->stmt)));
    return -1;
}

void db_statement_finalize(DbStatement *stmt) {
    if (!stmt) {
        return;
    }
    if (stmt->stmt) {
        sqlite3_finalize(stmt->stmt);
        stmt->stmt = NULL;
    }
    free(stmt);
}

int db_statement_bind_int(DbStatement *stmt, int index, int value) {
    int rc;

    if (!stmt || !stmt->stmt || index <= 0) {
        return -1;
    }

    rc = sqlite3_bind_int(stmt->stmt, index, value);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "db: bind_int failed: %s\n", sqlite3_errmsg(sqlite3_db_handle(stmt->stmt)));
        return -1;
    }
    return 0;
}

int db_statement_bind_text(DbStatement *stmt, int index, const char *value) {
    int rc;

    if (!stmt || !stmt->stmt || index <= 0) {
        return -1;
    }

    rc = sqlite3_bind_text(stmt->stmt, index, value ? value : "", -1, SQLITE_TRANSIENT);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "db: bind_text failed: %s\n", sqlite3_errmsg(sqlite3_db_handle(stmt->stmt)));
        return -1;
    }
    return 0;
}

int db_statement_column_int(DbStatement *stmt, int col_index) {
    if (!stmt || !stmt->stmt || col_index < 0) {
        return 0;
    }
    return sqlite3_column_int(stmt->stmt, col_index);
}

const char *db_statement_column_text(DbStatement *stmt, int col_index) {
    if (!stmt || !stmt->stmt || col_index < 0) {
        return NULL;
    }
    return (const char *)sqlite3_column_text(stmt->stmt, col_index);
}
