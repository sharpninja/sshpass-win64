/*
 * Minimal C test framework - zero dependencies
 * Usage:
 *   #include "test_framework.h"
 *   void test_something(void) { ASSERT_EQ(1, 1); }
 *   int main() { RUN_TEST(test_something); TEST_SUMMARY(); return test_failures; }
 */
#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <string.h>

static int test_count = 0;
static int test_failures = 0;
static int current_test_failed = 0;

#define RUN_TEST(func) do { \
    current_test_failed = 0; \
    test_count++; \
    func(); \
    if (current_test_failed) { \
        fprintf(stderr, "  FAIL: %s\n", #func); \
        test_failures++; \
    } else { \
        fprintf(stderr, "  PASS: %s\n", #func); \
    } \
} while(0)

#define ASSERT_TRUE(cond) do { \
    if (!(cond)) { \
        fprintf(stderr, "    ASSERT_TRUE failed: %s (line %d)\n", #cond, __LINE__); \
        current_test_failed = 1; \
        return; \
    } \
} while(0)

#define ASSERT_FALSE(cond) do { \
    if ((cond)) { \
        fprintf(stderr, "    ASSERT_FALSE failed: %s (line %d)\n", #cond, __LINE__); \
        current_test_failed = 1; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
    long long _e = (long long)(expected); \
    long long _a = (long long)(actual); \
    if (_e != _a) { \
        fprintf(stderr, "    ASSERT_EQ failed: expected %lld, got %lld (line %d)\n", _e, _a, __LINE__); \
        current_test_failed = 1; \
        return; \
    } \
} while(0)

#define ASSERT_NEQ(expected, actual) do { \
    long long _e = (long long)(expected); \
    long long _a = (long long)(actual); \
    if (_e == _a) { \
        fprintf(stderr, "    ASSERT_NEQ failed: both are %lld (line %d)\n", _e, __LINE__); \
        current_test_failed = 1; \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(expected, actual) do { \
    const char *_e = (expected); \
    const char *_a = (actual); \
    if (_e == NULL && _a == NULL) break; \
    if (_e == NULL || _a == NULL || strcmp(_e, _a) != 0) { \
        fprintf(stderr, "    ASSERT_STR_EQ failed: expected \"%s\", got \"%s\" (line %d)\n", \
                _e ? _e : "(null)", _a ? _a : "(null)", __LINE__); \
        current_test_failed = 1; \
        return; \
    } \
} while(0)

#define ASSERT_NOT_NULL(ptr) do { \
    if ((ptr) == NULL) { \
        fprintf(stderr, "    ASSERT_NOT_NULL failed: %s is NULL (line %d)\n", #ptr, __LINE__); \
        current_test_failed = 1; \
        return; \
    } \
} while(0)

#define ASSERT_NULL(ptr) do { \
    if ((ptr) != NULL) { \
        fprintf(stderr, "    ASSERT_NULL failed: %s is not NULL (line %d)\n", #ptr, __LINE__); \
        current_test_failed = 1; \
        return; \
    } \
} while(0)

#define TEST_SUMMARY() do { \
    fprintf(stderr, "\n%d/%d tests passed", test_count - test_failures, test_count); \
    if (test_failures > 0) \
        fprintf(stderr, " (%d FAILED)\n", test_failures); \
    else \
        fprintf(stderr, " (all passed)\n"); \
} while(0)

#endif /* TEST_FRAMEWORK_H */
