#ifndef ACTIVE_QUERY_H
#define ACTIVE_QUERY_H

#include "active_record.h"

typedef struct {
    ActiveRecordStore *store;
    const char *where_field;
    const char *where_value;
    int has_where;
    int limit; /* 0 or negative means no explicit limit */
} ActiveQuery;

void active_query_init(ActiveQuery *query, ActiveRecordStore *store);

/* Chainable filter by field/value (equality only). */
ActiveQuery *active_query_where(ActiveQuery *query, const char *field, const char *value);

/* Chainable limit on number of returned records. */
ActiveQuery *active_query_limit(ActiveQuery *query, int limit);

/* Execute the query against the in-memory store.
 * Returns a newly allocated array of ActiveModel* and sets *out_count.
 * Caller is responsible for freeing the returned array (but not the models).
 */
ActiveModel **active_query_execute(ActiveQuery *query, int *out_count);

#endif /* ACTIVE_QUERY_H */

