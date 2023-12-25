#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "leptjson.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)                \
    do {                                                                \
        test_count++;                                                   \
        if (equality)                                                   \
            test_pass++;                                                \
        else {                                                          \
            fprintf(stderr, "%s:%d: expect: " format " actual: " format "\n", __FILE__, __LINE__, expect, actual); \
            main_ret = 1;                                               \
        }                                                               \
    } while(0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g")

#define EXPECT_EQ_RESULT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), PARSE_RESULTS[expect], PARSE_RESULTS[actual], "%s")

#define EXPECT_EQ_TYPE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), LEPT_TYPES[expect], LEPT_TYPES[actual], "%s")

#define TEST_ERROR(error, json)                         \
    do {                                                \
        lept_value v;                                   \
        v.type = LEPT_FALSE;                            \
        EXPECT_EQ_RESULT(error, lept_parse(&v, json));  \
        EXPECT_EQ_TYPE(LEPT_NULL, lept_get_type(&v));   \
    } while(0)

#define TEST_NUMBER(expect, json)                               \
    do {                                                        \
        lept_value v;                                           \
        EXPECT_EQ_RESULT(LEPT_PARSE_OK, lept_parse(&v, json));  \
        EXPECT_EQ_TYPE(LEPT_NUMBER, lept_get_type(&v));         \
        EXPECT_EQ_DOUBLE(expect, lept_get_number(&v));          \
    } while(0)

static void test_parse_null() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_RESULT(LEPT_PARSE_OK, lept_parse(&v, "null"));
    EXPECT_EQ_TYPE(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_true() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_RESULT(LEPT_PARSE_OK, lept_parse(&v, "true"));
    EXPECT_EQ_TYPE(LEPT_TRUE, lept_get_type(&v));
}

static void test_parse_false() {
    lept_value v;
    v.type = LEPT_NULL;
    EXPECT_EQ_RESULT(LEPT_PARSE_OK, lept_parse(&v, "false"));
    EXPECT_EQ_TYPE(LEPT_FALSE, lept_get_type(&v));
}

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000"); /* must underflow */
}

static void test_parse_expect_value() {
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(LEPT_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_invalid_value() {
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nul");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "?");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "ture");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "fals");

#if 0
    /* invalid number */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, ".123"); /* at least one digit before '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "1.");   /* at least one digit after '.' */
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(LEPT_PARSE_INVALID_VALUE, "nan");
#endif
}

static void test_parse_root_not_singular() {
    lept_value v;
    v.type = LEPT_FALSE;
    EXPECT_EQ_INT(LEPT_PARSE_ROOT_NOT_SINGULAR, lept_parse(&v, "null x"));
    EXPECT_EQ_INT(LEPT_NULL, lept_get_type(&v));
}

static void test_parse_number_too_big() {
#if 0
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "1e309");
    TEST_ERROR(LEPT_PARSE_NUMBER_TOO_BIG, "-1e309");
#endif
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_number();
    test_parse_expect_value();
    test_parse_invalid_value();
    test_parse_root_not_singular();
    test_parse_number_too_big();
}

int main() {
    test_parse();
    printf("%d/%d (%3.2f) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}
