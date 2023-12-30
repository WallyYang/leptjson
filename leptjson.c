#include <assert.h> /* assert() */
#include <errno.h>  /* errno, ERANGE */
#include <math.h>   /* HUGE_VAL */
#include <stdlib.h> /* NULL, malloc(), realloc(), free(), strtod() */
#include <string.h> /* memcpy(), strlen() */

#include "leptjson.h"

#ifndef LEPT_PARSE_STACK_INIT_SIZE
#define LEPT_PARSE_STACK_INIT_SIZE 256
#endif

#define EXPECT(c, ch)  do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch)    do { *(char*)lept_context_push(c, sizeof(char)) = (ch); } while(0)

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
} lept_context;

const char* LEPT_TYPES[] = {
    "LEPT_NULL",
    "LEPT_FALSE",
    "LEPT_TRUE",
    "LEPT_NUMBER",
    "LEPT_STRING",
    "LEPT_ARRAY",
    "LEPT_OBJECT",
};

const char* PARSE_RESULTS[] = {
    "LEPT_PARSE_OK",
    "LEPT_PARSE_EXPECT_VALUE",
    "LEPT_PARSE_INVALID_VALUE",
    "LEPT_PARSE_ROOT_NOT_SINGULAR",
    "LEPT_PARSE_NUMBER_TOO_BIG",
    "LEPT_PARSE_MISS_QUOTATION_MARK",
    "LEPT_PARSE_INVALID_STRING_ESCAPE",
    "LEPT_PARSE_INVALID_STRING_CHAR",
};

static void* lept_context_push(lept_context* c, size_t size) {
    void* ret;
    assert(size > 0);
    if (c->top + size >= c->size) {
        if (c->size == 0)
            c->size = LEPT_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1; /* c->size * 1.5 */
        c->stack = (char*)realloc(c->stack, c->size);
    }
    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* lept_context_pop(lept_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\r')
        p++;
    c->json = p;
}

static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; i < strlen(literal)-1; ++i) {
        if (c->json[i] != literal[i+1]) {
            return LEPT_PARSE_INVALID_VALUE;
        }
    }
    c->json += strlen(literal)-1;
    v->type = type;
    return LEPT_PARSE_OK;
}

static int lept_parse_number(lept_context* c, lept_value* v) {
    const char *p = c->json;
    if (*p == '-') ++p;
    if (*p == '0') ++p;
    else {
        if (!ISDIGIT1TO9(*p)) return LEPT_PARSE_INVALID_VALUE;
        while (ISDIGIT(*p)) ++p;
    }
    if (*p == '.') {
        ++p;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        while (ISDIGIT(*p)) ++p;
    }
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '+' || *p == '-') ++p;
        if (!ISDIGIT(*p)) return LEPT_PARSE_INVALID_VALUE;
        while (ISDIGIT(*p)) ++p;
    }
    errno = 0;
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    c->json = p;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_string(lept_context* c, lept_value* v) {
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
        case '\"':
            len = c->top - head;
            lept_set_string(v, (const char*)lept_context_pop(c, len), len);
            c->json = p;
            return LEPT_PARSE_OK;
        case '\\':
            ch = *p++;
            switch (ch) {
            case '"':  PUTC(c, '"');  break;
            case '\\': PUTC(c, '\\'); break;
            case '/':  PUTC(c, '/');  break;
            case 'b':  PUTC(c, '\b'); break;
            case 'f':  PUTC(c, '\f'); break;
            case 'n':  PUTC(c, '\n'); break;
            case 'r':  PUTC(c, '\r'); break;
            case 't':  PUTC(c, '\t'); break;
            default:
                c ->top = head;
                return LEPT_PARSE_INVALID_STRING_ESCAPE;
            }
            break;
        case '\0':
            c->top = head;
            return LEPT_PARSE_MISS_QUOTATION_MARK;
        default:
            if (ch < '\x20') {
                c->top = head;
                return LEPT_PARSE_INVALID_STRING_CHAR;
            }
            PUTC(c, ch);
        }
    }
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
    case 'n': return lept_parse_literal(c, v, "null", LEPT_NULL);
    case 't': return lept_parse_literal(c, v, "true", LEPT_TRUE);
    case 'f': return lept_parse_literal(c, v, "false", LEPT_FALSE);
    default:  return lept_parse_number(c, v);
    case '"': return lept_parse_string(c, v);
    case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value *v, const char *json) {
    lept_context c;
    int result;
    assert(v != NULL);

    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    lept_init(v);
    lept_parse_whitespace(&c);

    if ((result = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    assert(c.top == 0);
    free(c.stack);
    return result;
}

void lept_free(lept_value* v) {
    assert(v != NULL);
    if (v->type == LEPT_STRING)
        free(v->u.s.s);
    v->type = LEPT_NULL;
}

lept_type lept_get_type(const lept_value *v) {
    assert(v != NULL);
    return v->type;
}

int lept_get_boolean(const lept_value *v) {
    assert(v != NULL && (v->type == LEPT_TRUE || v->type == LEPT_FALSE));
    return v->type == LEPT_TRUE;
}

void lept_set_boolean(lept_value *v, int b) {
    assert(v != NULL && (b == 0 || b == 1));
    lept_free(v);
    v->type = b? LEPT_TRUE : LEPT_FALSE;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v-> u.n;
}

void lept_set_number(lept_value *v, double n) {
    assert(v != NULL);
    lept_free(v);
    v->u.n = n;
    v->type = LEPT_NUMBER;
}

const char* lept_get_string(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.s;
}

size_t lept_get_string_length(const lept_value *v) {
    assert(v != NULL && v->type == LEPT_STRING);
    return v->u.s.len;
}

void lept_set_string(lept_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    lept_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = LEPT_STRING;
}
