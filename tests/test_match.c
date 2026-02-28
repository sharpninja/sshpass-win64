/*
 * Unit tests for the match() function from sshpass
 *
 * match() is a streaming string matcher that maintains state across calls.
 * It returns the current match position in the reference string.
 * When reference[state] == '\0', a full match has been found.
 */

#include "test_framework.h"
#include <stdlib.h>

/* Declaration of the function under test */
int match(const char *reference, const char *buffer, int bufsize, int state);

/* Test: Full match in a single buffer */
void test_match_full_single_buffer(void)
{
    const char *ref = "assword";
    const char *buf = "Password: ";
    int state = match(ref, buf, (int)strlen(buf), 0);
    /* "assword" should be fully matched within "Password: " */
    ASSERT_EQ(strlen(ref), state);
    ASSERT_EQ('\0', ref[state]);
}

/* Test: No match at all */
void test_match_no_match(void)
{
    const char *ref = "assword";
    const char *buf = "Hello World";
    int state = match(ref, buf, (int)strlen(buf), 0);
    ASSERT_TRUE(ref[state] != '\0');
}

/* Test: Partial match across two buffers */
void test_match_split_across_buffers(void)
{
    const char *ref = "assword";
    /* First buffer has "Pass" - matches "ass" (3 chars of ref) */
    const char *buf1 = "Pass";
    int state = match(ref, buf1, (int)strlen(buf1), 0);
    ASSERT_EQ(3, state); /* matched "ass" */

    /* Second buffer has "word: " - should complete the match */
    const char *buf2 = "word: ";
    state = match(ref, buf2, (int)strlen(buf2), state);
    ASSERT_EQ(strlen(ref), state);
    ASSERT_EQ('\0', ref[state]);
}

/* Test: Match at the very beginning of buffer */
void test_match_at_beginning(void)
{
    const char *ref = "Hello";
    const char *buf = "Hello World";
    int state = match(ref, buf, (int)strlen(buf), 0);
    ASSERT_EQ(strlen(ref), state);
    ASSERT_EQ('\0', ref[state]);
}

/* Test: Match at the very end of buffer */
void test_match_at_end(void)
{
    const char *ref = "end";
    const char *buf = "the end";
    int state = match(ref, buf, (int)strlen(buf), 0);
    ASSERT_EQ(strlen(ref), state);
    ASSERT_EQ('\0', ref[state]);
}

/* Test: Single character reference */
void test_match_single_char(void)
{
    const char *ref = "x";
    const char *buf = "abcxyz";
    int state = match(ref, buf, (int)strlen(buf), 0);
    ASSERT_EQ(1, state);
    ASSERT_EQ('\0', ref[state]);
}

/* Test: Case sensitivity - should NOT match different case */
void test_match_case_sensitive(void)
{
    const char *ref = "password";
    const char *buf = "PASSWORD";
    int state = match(ref, buf, (int)strlen(buf), 0);
    ASSERT_TRUE(ref[state] != '\0'); /* Should not match */
}

/* Test: Empty buffer */
void test_match_empty_buffer(void)
{
    const char *ref = "test";
    const char *buf = "";
    int state = match(ref, buf, 0, 0);
    ASSERT_EQ(0, state);
}

/* Test: Match with repeated partial patterns */
void test_match_repeated_partial(void)
{
    const char *ref = "abc";
    /* "ababc" - starts matching "ab", then 'a' breaks it, restarts, matches "abc" */
    const char *buf = "ababc";
    int state = match(ref, buf, (int)strlen(buf), 0);
    ASSERT_EQ(strlen(ref), state);
    ASSERT_EQ('\0', ref[state]);
}

/* Test: Multiple calls with single characters */
void test_match_char_by_char(void)
{
    const char *ref = "test";
    int state = 0;
    state = match(ref, "t", 1, state);
    ASSERT_EQ(1, state);
    state = match(ref, "e", 1, state);
    ASSERT_EQ(2, state);
    state = match(ref, "s", 1, state);
    ASSERT_EQ(3, state);
    state = match(ref, "t", 1, state);
    ASSERT_EQ(4, state);
    ASSERT_EQ('\0', ref[state]);
}

/* Test: The actual SSH password prompt pattern */
void test_match_ssh_password_prompt(void)
{
    const char *ref = "assword";
    /* Simulate SSH output arriving in chunks */
    int state = 0;
    state = match(ref, "user@host's p", 13, state);
    ASSERT_TRUE(ref[state] != '\0'); /* Not yet matched */
    state = match(ref, "assword: ", 9, state);
    ASSERT_EQ(strlen(ref), state);
    ASSERT_EQ('\0', ref[state]);
}

int main(void)
{
    fprintf(stderr, "=== match() unit tests ===\n");

    RUN_TEST(test_match_full_single_buffer);
    RUN_TEST(test_match_no_match);
    RUN_TEST(test_match_split_across_buffers);
    RUN_TEST(test_match_at_beginning);
    RUN_TEST(test_match_at_end);
    RUN_TEST(test_match_single_char);
    RUN_TEST(test_match_case_sensitive);
    RUN_TEST(test_match_empty_buffer);
    RUN_TEST(test_match_repeated_partial);
    RUN_TEST(test_match_char_by_char);
    RUN_TEST(test_match_ssh_password_prompt);

    TEST_SUMMARY();
    return test_failures;
}
