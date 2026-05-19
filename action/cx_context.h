#ifndef CX_CONTEXT_H
#define CX_CONTEXT_H

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#define CX_BUF_SIZE (256 * 1024)
#define CX_VAR_MAX  64
#define CX_KEY_MAX  64
#define CX_VAL_MAX  512

typedef struct {
    char key[CX_KEY_MAX];
    char val[CX_VAL_MAX];
    int  is_int;
    int  int_val;
} CxVar;

typedef struct CxContext {
    char  buf[CX_BUF_SIZE];
    int   buf_len;

    CxVar vars[CX_VAR_MAX];
    int   var_count;

    const char *layout;

    char  yield_buf[CX_BUF_SIZE];
    int   yield_len;
} CxContext;

void cx_init(CxContext *cx);

void cx_write(CxContext *cx, const char *s, int len);
void cx_writef(CxContext *cx, const char *fmt, ...);

void cx_write_escaped(CxContext *cx, const char *s);
void cx_writef_escaped(CxContext *cx, const char *fmt, ...);

const char *cx_get(CxContext *cx, const char *key);
int         cx_get_int(CxContext *cx, const char *key);
int         cx_isset(CxContext *cx, const char *key);

void cx_set(CxContext *cx, const char *key, const char *val);
void cx_set_int(CxContext *cx, const char *key, int val);
void cx_set_fmt(CxContext *cx, const char *key, const char *fmt, ...);

void cx_set_layout(CxContext *cx, const char *layout);
const char *cx_yield(CxContext *cx);

#endif /* CX_CONTEXT_H */
