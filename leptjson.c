#include <assert.h> /* assert() */
#include <errno.h>  /* errno */
#include <math.h>   /* HUGE_VAL */
#include <stdlib.h> /* NULL, strtod() */
#include <string.h> /* strlen() */

#include "leptjson.h"

#define EXPECT(c, ch)  do { assert(*c->json == (ch)); c->json++; } while(0)

#define ISDIGIT(ch)         ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch)     ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
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
};

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
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    c->json = p;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
    case 'n': return lept_parse_literal(c, v, "null", LEPT_NULL);
    case 't': return lept_parse_literal(c, v, "true", LEPT_TRUE);
    case 'f': return lept_parse_literal(c, v, "false", LEPT_FALSE);
    default:  return lept_parse_number(c, v);
    case '\0': return LEPT_PARSE_EXPECT_VALUE;
    }
}

int lept_parse(lept_value *v, const char *json) {
    lept_context c;
    int result;
    assert(v != NULL);

    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);

    if ((result = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            return LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return result;
}

lept_type lept_get_type(const lept_value *v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v-> n;
}
