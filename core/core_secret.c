#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "core_secret.h"

#define RAW_MIN_BYTES 64
#define HEX_LEN ((size_t)(RAW_MIN_BYTES)*2)
#define CACHE_MAX 8192
#define SECRET_KEY_FILE "config/secret.key"

static int g_initialized;
static int g_init_ok;
static char g_cached[CACHE_MAX];

static int env_is_production(void) {
    const char *env = getenv("CORE_ENV");

    if (!env || env[0] == '\0') {
        return 0;
    }
    return strcasecmp(env, "production") == 0 ? 1 : 0;
}

static int chars_all_hex(const char *s, size_t len) {
    size_t i;

    for (i = 0; i < len; ++i) {
        char c = s[i];
        int ok = ((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'));
        if (!ok) {
            return 0;
        }
    }
    return 1;
}

static int strip_copy_trimmed_line(const char *src, size_t src_len, char *out, size_t out_sz) {
    size_t start = 0;
    size_t end = src_len;
    size_t trimmed;

    while (start < end && (src[start] == ' ' || src[start] == '\t')) {
        start++;
    }
    while (end > start && (src[end - 1] == ' ' || src[end - 1] == '\t' || src[end - 1] == '\n' ||
                           src[end - 1] == '\r')) {
        end--;
    }
    trimmed = end - start;
    if (trimmed >= out_sz) {
        return -1;
    }
    memcpy(out, src + start, trimmed);
    out[trimmed] = '\0';
    return 0;
}

/* 128 hex chars (512 bits as hex digits) OR opaque string long enough (>= 43 ~ 64 bytes base64). */
static int validate_secret_material(const char *s) {
    size_t len;

    if (!s) {
        return 0;
    }
    len = strlen(s);
    if (len >= HEX_LEN && (len % 2U) == 0U && chars_all_hex(s, len)) {
        return 1;
    }
    if (len >= 43U && len < CACHE_MAX - 4U) {
        return 1;
    }
    return 0;
}

static void copy_into_cache(const char *s) {
    size_t len = strlen(s);
    memset(g_cached, 0, sizeof(g_cached));
    memcpy(g_cached, s, len + 1U);
}

static int raw_to_hex_string(const unsigned char *raw, size_t n, char *out, size_t out_sz) {
    static const char *xd = "0123456789abcdef";
    size_t i;

    if (out_sz < n * 2U + 1U) {
        return -1;
    }
    for (i = 0; i < n; ++i) {
        out[i * 2U] = xd[(raw[i] >> 4U) & 0xFU];
        out[i * 2U + 1U] = xd[raw[i] & 0xFU];
    }
    out[n * 2U] = '\0';
    return 0;
}

static int read_entire_trimmed_secret_file(FILE *fp, char *out, size_t out_sz, char *scratch, size_t scratch_sz) {
    size_t nread;

    nread = fread(scratch, 1U, scratch_sz - 1U, fp);
    if (ferror(fp)) {
        return -1;
    }
    scratch[nread] = '\0';
    return strip_copy_trimmed_line(scratch, nread, out, out_sz);
}

static int read_secret_file(const char *path, char *out, size_t out_sz, char *scratch, size_t scratch_sz) {
    FILE *fp;

    fp = fopen(path, "rb");
    if (!fp) {
        return -1;
    }
    if (read_entire_trimmed_secret_file(fp, out, out_sz, scratch, scratch_sz) != 0) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return validate_secret_material(out) ? 0 : -1;
}

static int read_urandom_bytes(unsigned char *buf, size_t len) {
    int fd;
    size_t off = 0;

    fd = open("/dev/urandom", O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    while (off < len) {
        ssize_t n = read(fd, buf + off, len - off);
        if (n <= 0) {
            close(fd);
            return -1;
        }
        off += (size_t)n;
    }
    close(fd);
    return 0;
}

static int write_hex_material(const char *filepath, const unsigned char raw[RAW_MIN_BYTES]) {
    static const char *xdigits = "0123456789abcdef";
    FILE *fp;
    size_t i;

    fp = fopen(filepath, "wb");
    if (!fp) {
        return -1;
    }
    for (i = 0; i < RAW_MIN_BYTES; ++i) {
        if (putc((int)(unsigned char)(xdigits[(raw[i] >> 4U) & 0xFU]), fp) == EOF ||
            putc((int)(unsigned char)(xdigits[raw[i] & 0xFU]), fp) == EOF) {
            fclose(fp);
            return -1;
        }
    }
    if (putc('\n', fp) == EOF) {
        fclose(fp);
        return -1;
    }
    if (fflush(fp) != 0) {
        fclose(fp);
        return -1;
    }
    fclose(fp);
#ifndef _WIN32
    chmod(filepath, S_IRUSR | S_IWUSR);
#endif
    return 0;
}

int cortex_secret_write_new_project_keyfile(const char *filepath) {
    unsigned char raw[RAW_MIN_BYTES];

    if (!filepath || filepath[0] == '\0') {
        return -1;
    }
    if (read_urandom_bytes(raw, sizeof(raw)) != 0) {
        fprintf(stderr, "cortex_secret: failed to read /dev/urandom\n");
        return -1;
    }
    if (write_hex_material(filepath, raw) != 0) {
        fprintf(stderr, "cortex_secret: failed to write key file \"%s\"\n", filepath);
        return -1;
    }
    return 0;
}

void cortex_secret_reset_for_testing(void) {
    g_initialized = 0;
    g_init_ok = 0;
    memset(g_cached, 0, sizeof(g_cached));
}

const char *get_secret_key_base(void) {
    if (!g_init_ok || g_cached[0] == '\0') {
        return NULL;
    }
    return g_cached;
}

static int persist_dev_key_once(const unsigned char raw[RAW_MIN_BYTES]) {
    if (write_hex_material(SECRET_KEY_FILE, raw) != 0) {
        fprintf(stderr, "cortex_secret: failed to write '%s'\n", SECRET_KEY_FILE);
        return -1;
    }
    fprintf(stderr, "cortex: wrote config/secret.key (development)\n");
    return 0;
}

int cortex_secret_init(void) {
    const char *env_secret;
    char file_buf[CACHE_MAX];
    char scratch[CACHE_MAX];
    unsigned char raw[RAW_MIN_BYTES];

    if (g_initialized) {
        return g_init_ok ? 0 : -1;
    }
    g_initialized = 1;
    g_init_ok = 0;

    env_secret = getenv("SECRET_KEY_BASE");
    if (env_secret && env_secret[0] != '\0') {
        if (strip_copy_trimmed_line(env_secret, strlen(env_secret), file_buf, sizeof(file_buf)) != 0 ||
            !validate_secret_material(file_buf)) {
            fprintf(stderr, "cortex_secret: SECRET_KEY_BASE is missing or invalid (see config/secret.key format)\n");
            return -1;
        }
        copy_into_cache(file_buf);
        g_init_ok = 1;
        return 0;
    }

    if (read_secret_file(SECRET_KEY_FILE, file_buf, sizeof(file_buf), scratch, sizeof(scratch)) == 0) {
        copy_into_cache(file_buf);
        g_init_ok = 1;
        return 0;
    }

    if (env_is_production()) {
        fprintf(stderr,
                "cortex_secret: production requires SECRET_KEY_BASE or config/secret.key "
                "(set SECRET_KEY_BASE or add config/secret.key from `cortex new`)\n");
        return -1;
    }

    if (read_urandom_bytes(raw, sizeof(raw)) != 0) {
        fprintf(stderr, "cortex_secret: failed to read /dev/urandom\n");
        return -1;
    }
    if (raw_to_hex_string(raw, sizeof(raw), file_buf, sizeof(file_buf)) != 0) {
        return -1;
    }
    copy_into_cache(file_buf);
    g_init_ok = 1;

    if (persist_dev_key_once(raw) != 0) {
        /* In-memory secret is usable; file write failed (e.g. read-only cwd). */
    }
    return 0;
}
