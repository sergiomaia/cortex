#ifndef POSTGRES_ADAPTER_H
#define POSTGRES_ADAPTER_H

#ifdef CORTEX_HAVE_POSTGRES

#include "../db_connection.h"

/*
 * Creates a PostgreSQL-backed DbConnection.
 * Supports libpq connection string or URI syntax.
 */
DbConnection *postgres_connect(const char *connstring);

/*
 * Connects using PostgreSQL environment variables:
 *   PGHOST
 *   PGPORT
 *   PGDATABASE
 *   PGUSER
 *   PGPASSWORD
 *
 * With CORE_ENV fallbacks.
 */
DbConnection *postgres_connect_from_env(void);

/* Registers PostgreSQL as the default adapter */
void postgres_adapter_register(void);

#endif /* CORTEX_HAVE_POSTGRES */
#endif /* POSTGRES_ADAPTER_H */
