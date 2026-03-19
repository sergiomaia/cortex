#ifndef DB_MIGRATION_GENERATOR_H
#define DB_MIGRATION_GENERATOR_H

/* Generate a timestamped migration file under `db/migrations/`.
 *
 * Produces:
 *   db/migrations/%03d_<slug>.c
 *
 * The generated file includes:
 *   - a timestamp marker comment
 *   - `int up(void)` stub
 *   - `int down(void)` stub
 */
int db_generate_migration(int version, const char *slug);

#endif /* DB_MIGRATION_GENERATOR_H */

