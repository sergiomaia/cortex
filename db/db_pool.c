#include "db_pool.h"

#include <stdio.h>
#include <string.h>

#include "../core/core_config.h"
#include "db_paths.h"

static DbPool g_db_pool;
static int g_db_pool_initialized;
static char g_db_pool_path[512];

static int db_pool_is_valid_pragma_value(const char *value) {
    size_t i;
    if (!value || value[0] == '\0') {
        return 0;
    }
    for (i = 0; value[i] != '\0'; i++) {
        char c = value[i];
        if (!((c >= 'A' && c <= 'Z') || c == '_')) {
            return 0;
        }
    }
    return 1;
}

static int db_pool_apply_pragmas(DbConnection *conn) {
    const char *journal_mode;
    const char *sync_mode;
    int timeout_ms;
    char sql[128];

    if (!conn) {
        return -1;
    }

    journal_mode = core_config_get_string("db.journal_mode", "WAL");
    if (!db_pool_is_valid_pragma_value(journal_mode)) {
        journal_mode = "WAL";
    }
    sync_mode = core_config_get_string("db.synchronous", "NORMAL");
    if (!db_pool_is_valid_pragma_value(sync_mode)) {
        sync_mode = "NORMAL";
    }
    timeout_ms = core_config_get_int("db.pool_timeout_ms", 5000);
    if (timeout_ms <= 0) {
        timeout_ms = 5000;
    }

    if (snprintf(sql, sizeof(sql), "PRAGMA journal_mode=%s;", journal_mode) >= (int)sizeof(sql)) {
        return -1;
    }
    if (db_connection_exec(conn, sql) != 0) {
        return -1;
    }

    if (snprintf(sql, sizeof(sql), "PRAGMA synchronous=%s;", sync_mode) >= (int)sizeof(sql)) {
        return -1;
    }
    if (db_connection_exec(conn, sql) != 0) {
        return -1;
    }

    if (snprintf(sql, sizeof(sql), "PRAGMA busy_timeout=%d;", timeout_ms) >= (int)sizeof(sql)) {
        return -1;
    }
    if (db_connection_exec(conn, sql) != 0) {
        return -1;
    }
    return 0;
}

static int db_pool_clamp_size(int size) {
    if (size <= 0) {
        return DB_POOL_DEFAULT_SIZE;
    }
    if (size < 1) {
        return 1;
    }
    if (size > DB_POOL_MAX_SIZE) {
        return DB_POOL_MAX_SIZE;
    }
    return size;
}

int db_pool_init(DbPool *pool, const char *db_path, int size) {
    int i;
    int real_size;

    if (!pool || !db_path || db_path[0] == '\0') {
        return -1;
    }

    real_size = db_pool_clamp_size(size);
    memset(pool, 0, sizeof(*pool));

    if (pthread_mutex_init(&pool->pool_lock, NULL) != 0) {
        return -1;
    }
    if (pthread_cond_init(&pool->slot_available, NULL) != 0) {
        (void)pthread_mutex_destroy(&pool->pool_lock);
        return -1;
    }

    pool->size = real_size;
    pool->db_path = db_path;

    for (i = 0; i < pool->size; i++) {
        if (pthread_mutex_init(&pool->slots[i].lock, NULL) != 0) {
            int j;
            for (j = 0; j < i; j++) {
                if (pool->slots[j].conn) {
                    db_connection_close(pool->slots[j].conn);
                    pool->slots[j].conn = NULL;
                }
                (void)pthread_mutex_destroy(&pool->slots[j].lock);
            }
            (void)pthread_cond_destroy(&pool->slot_available);
            (void)pthread_mutex_destroy(&pool->pool_lock);
            return -1;
        }

        if (db_connection_open(db_path, &pool->slots[i].conn) != 0) {
            int j;
            (void)pthread_mutex_destroy(&pool->slots[i].lock);
            for (j = 0; j < i; j++) {
                if (pool->slots[j].conn) {
                    db_connection_close(pool->slots[j].conn);
                    pool->slots[j].conn = NULL;
                }
                (void)pthread_mutex_destroy(&pool->slots[j].lock);
            }
            (void)pthread_cond_destroy(&pool->slot_available);
            (void)pthread_mutex_destroy(&pool->pool_lock);
            return -1;
        }

        if (db_pool_apply_pragmas(pool->slots[i].conn) != 0) {
            int j;
            db_connection_close(pool->slots[i].conn);
            pool->slots[i].conn = NULL;
            (void)pthread_mutex_destroy(&pool->slots[i].lock);
            for (j = 0; j < i; j++) {
                if (pool->slots[j].conn) {
                    db_connection_close(pool->slots[j].conn);
                    pool->slots[j].conn = NULL;
                }
                (void)pthread_mutex_destroy(&pool->slots[j].lock);
            }
            (void)pthread_cond_destroy(&pool->slot_available);
            (void)pthread_mutex_destroy(&pool->pool_lock);
            return -1;
        }
    }

    return 0;
}

void db_pool_shutdown(DbPool *pool) {
    int i;

    if (!pool || pool->size <= 0) {
        return;
    }

    if (pthread_mutex_lock(&pool->pool_lock) != 0) {
        return;
    }

    for (;;) {
        int any_in_use = 0;
        for (i = 0; i < pool->size; i++) {
            if (pool->slots[i].in_use) {
                any_in_use = 1;
                break;
            }
        }
        if (!any_in_use) {
            break;
        }
        (void)pthread_cond_wait(&pool->slot_available, &pool->pool_lock);
    }

    for (i = 0; i < pool->size; i++) {
        DbConnection *conn = pool->slots[i].conn;
        pool->slots[i].conn = NULL;
        pool->slots[i].in_use = 0;
        if (conn) {
            db_connection_close(conn);
        }
    }
    (void)pthread_mutex_unlock(&pool->pool_lock);

    for (i = 0; i < pool->size; i++) {
        (void)pthread_mutex_destroy(&pool->slots[i].lock);
    }
    (void)pthread_cond_destroy(&pool->slot_available);
    (void)pthread_mutex_destroy(&pool->pool_lock);
    pool->size = 0;
    pool->db_path = NULL;
}

DbConnection *db_pool_acquire(DbPool *pool) {
    int i;

    if (!pool || pool->size <= 0) {
        return NULL;
    }

    if (pthread_mutex_lock(&pool->pool_lock) != 0) {
        return NULL;
    }

    for (;;) {
        for (i = 0; i < pool->size; i++) {
            if (!pool->slots[i].in_use && pool->slots[i].conn) {
                DbConnection *conn = pool->slots[i].conn;
                pool->slots[i].in_use = 1;
                (void)pthread_mutex_unlock(&pool->pool_lock);
                return conn;
            }
        }
        (void)pthread_cond_wait(&pool->slot_available, &pool->pool_lock);
    }
}

void db_pool_release(DbPool *pool, DbConnection *conn) {
    int i;

    if (!pool || !conn || pool->size <= 0) {
        return;
    }

    if (pthread_mutex_lock(&pool->pool_lock) != 0) {
        return;
    }

    for (i = 0; i < pool->size; i++) {
        if (pool->slots[i].conn == conn) {
            if (pool->slots[i].in_use) {
                pool->slots[i].in_use = 0;
                (void)pthread_cond_signal(&pool->slot_available);
            }
            break;
        }
    }
    (void)pthread_mutex_unlock(&pool->pool_lock);
}

int cortex_db_pool_init(int size) {
    if (g_db_pool_initialized) {
        return 0;
    }

    if (db_path_for_environment(NULL, g_db_pool_path, sizeof(g_db_pool_path)) != 0) {
        return -1;
    }

    if (db_pool_init(&g_db_pool, g_db_pool_path, size) != 0) {
        fprintf(stderr, "db: failed to initialize connection pool for '%s'\n", g_db_pool_path);
        return -1;
    }
    g_db_pool_initialized = 1;
    return 0;
}

void cortex_db_pool_shutdown(void) {
    if (!g_db_pool_initialized) {
        return;
    }
    db_pool_shutdown(&g_db_pool);
    g_db_pool_initialized = 0;
}

DbConnection *cortex_db_pool_acquire(void) {
    if (!g_db_pool_initialized) {
        return NULL;
    }
    return db_pool_acquire(&g_db_pool);
}

void cortex_db_pool_release(DbConnection *conn) {
    if (!g_db_pool_initialized) {
        return;
    }
    db_pool_release(&g_db_pool, conn);
}
