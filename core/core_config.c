#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "core_config.h"

typedef struct {
    char key[64];
    char value[128];
} CoreConfigEntry;

static CoreConfigEntry g_db_entries[16];
static int g_db_entry_count;
static int g_db_loaded;

static void cfg_copy(char *dst, size_t dst_size, const char *src) {
    size_t n;
    if (!dst || dst_size == 0) {
        return;
    }
    if (!src) {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= dst_size) {
        n = dst_size - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

static void cfg_trim(char *s) {
    char *start = s;
    char *end;
    size_t len;

    if (!s) {
        return;
    }

    while (*start && isspace((unsigned char)*start)) {
        start++;
    }
    if (start != s) {
        memmove(s, start, strlen(start) + 1);
    }

    len = strlen(s);
    if (len == 0) {
        return;
    }
    end = s + len - 1;
    while (end >= s && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }
}

static void core_config_load_db_ini(void) {
    FILE *f;
    char line[256];
    int in_db_section = 0;

    if (g_db_loaded) {
        return;
    }
    g_db_loaded = 1;
    g_db_entry_count = 0;

    f = fopen("config/database.ini", "r");
    if (!f) {
        return;
    }

    while (fgets(line, sizeof(line), f) != NULL) {
        char *eq;
        char *key;
        char *value;

        cfg_trim(line);
        if (line[0] == '\0' || line[0] == '#' || line[0] == ';') {
            continue;
        }

        if (line[0] == '[') {
            in_db_section = (strcmp(line, "[db]") == 0);
            continue;
        }
        if (!in_db_section) {
            continue;
        }

        eq = strchr(line, '=');
        if (!eq) {
            continue;
        }
        *eq = '\0';
        key = line;
        value = eq + 1;
        cfg_trim(key);
        cfg_trim(value);

        if (key[0] == '\0' || g_db_entry_count >= (int)(sizeof(g_db_entries) / sizeof(g_db_entries[0]))) {
            continue;
        }

        cfg_copy(g_db_entries[g_db_entry_count].key, sizeof(g_db_entries[g_db_entry_count].key), key);
        cfg_copy(g_db_entries[g_db_entry_count].value, sizeof(g_db_entries[g_db_entry_count].value), value);
        g_db_entry_count++;
    }

    fclose(f);
}

const char *core_config_get_string(const char *key, const char *default_value) {
    int i;

    if (!key || key[0] == '\0') {
        return default_value;
    }

    core_config_load_db_ini();

    if (strncmp(key, "db.", 3) != 0) {
        return default_value;
    }

    key += 3;
    for (i = 0; i < g_db_entry_count; i++) {
        if (strcmp(g_db_entries[i].key, key) == 0) {
            return g_db_entries[i].value;
        }
    }

    return default_value;
}

int core_config_get_int(const char *key, int default_value) {
    const char *value = core_config_get_string(key, NULL);
    long parsed;
    char *endptr;

    if (!value || value[0] == '\0') {
        return default_value;
    }

    parsed = strtol(value, &endptr, 10);
    if (endptr == value || *endptr != '\0') {
        return default_value;
    }
    return (int)parsed;
}

CoreConfig core_config_load(void) {
    CoreConfig cfg;
    const char *env;

    env = getenv("CORE_ENV");
    if (!env || env[0] == '\0') {
        cfg.environment = "development";
    } else {
        cfg.environment = env;
    }

    env = getenv("CORE_PORT");
    if (!env || env[0] == '\0') {
        cfg.port = 3000;
    } else {
        int port = atoi(env);
        if (port <= 0) {
            cfg.port = 3000;
        } else {
            cfg.port = port;
        }
    }

    return cfg;
}

