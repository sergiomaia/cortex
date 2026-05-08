#ifndef DB_POOL_H
#define DB_POOL_H

#include "db_connection.h"
#include <pthread.h>

#define DB_POOL_MAX_SIZE 8
#define DB_POOL_DEFAULT_SIZE 4

typedef struct {
    DbConnection *conn;
    int in_use;
    pthread_mutex_t lock;
} DbPoolSlot;

typedef struct {
    DbPoolSlot slots[DB_POOL_MAX_SIZE];
    int size;
    pthread_mutex_t pool_lock;
    pthread_cond_t slot_available;
    const char *db_path;
} DbPool;

/* Lifecycle */
int db_pool_init(DbPool *pool, const char *db_path, int size);
void db_pool_shutdown(DbPool *pool);

/* Acquire / release */
DbConnection *db_pool_acquire(DbPool *pool);
void db_pool_release(DbPool *pool, DbConnection *conn);

/* Global pool API */
int cortex_db_pool_init(int size);
void cortex_db_pool_shutdown(void);
DbConnection *cortex_db_pool_acquire(void);
void cortex_db_pool_release(DbConnection *conn);

#endif /* DB_POOL_H */
