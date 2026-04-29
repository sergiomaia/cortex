#ifndef CORE_SECRET_H
#define CORE_SECRET_H

/* Application secret (Rails-like secret_key_base). Load once per process via
 * cortex_secret_init(); then use get_secret_key_base() everywhere a stable
 * signing/crypto key string is needed.
 */

/* Bootstraps SECRET_KEY_BASE from getenv("SECRET_KEY_BASE") first, otherwise
 * config/secret.key (paths relative to the process current working directory).
 * Development / test (any CORE_ENV except production): if missing, generates
 * 64 cryptographically secure random bytes (hex-encoded) and writes
 * config/secret.key with mode 0600.
 * Production: fails (returns -1) if neither ENV nor file provides a valid key.
 *
 * Safe to call once at startup only; caches the result permanently.
 */
int cortex_secret_init(void);

/* Cached secret material (trimmed NUL-terminated line). Always the same pointer
 * after successful cortex_secret_init(). Returns NULL before init succeeds or if
 * init was never called / failed.
 */
const char *get_secret_key_base(void);

/* Generate config/secret.key for new projects (cortex new). Writes 128 hex chars
 * (64 random bytes). chmod 0600 on POSIX. Does not change process-global cache.
 */
int cortex_secret_write_new_project_keyfile(const char *filepath);

/* Clears cached state for unit tests — do not use in application code. */
void cortex_secret_reset_for_testing(void);

#endif /* CORE_SECRET_H */
