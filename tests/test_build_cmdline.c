/*
 * Unit tests for the build_command_line() function from sshpass
 *
 * build_command_line() takes argc/argv and produces a single command line
 * string suitable for CreateProcess, quoting arguments that contain spaces.
 */

#include "test_framework.h"
#include <stdlib.h>
#include <windows.h>

/* Declaration - when compiled with TESTING, this is non-static */
char *build_command_line(int argc, char *argv[]);

/* Test: Single simple argument */
void test_single_arg(void)
{
    char *argv[] = { "ssh" };
    char *result = build_command_line(1, argv);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ("ssh", result);
    free(result);
}

/* Test: Multiple simple arguments */
void test_multiple_args(void)
{
    char *argv[] = { "ssh", "-l", "user", "host" };
    char *result = build_command_line(4, argv);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ("ssh -l user host", result);
    free(result);
}

/* Test: Argument with spaces gets quoted */
void test_arg_with_spaces(void)
{
    char *argv[] = { "ssh", "my host" };
    char *result = build_command_line(2, argv);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ("ssh \"my host\"", result);
    free(result);
}

/* Test: Argument with tab gets quoted */
void test_arg_with_tab(void)
{
    char *argv[] = { "cmd", "has\ttab" };
    char *result = build_command_line(2, argv);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ("cmd \"has\ttab\"", result);
    free(result);
}

/* Test: Empty string argument gets quoted */
void test_empty_arg(void)
{
    char *argv[] = { "cmd", "" };
    char *result = build_command_line(2, argv);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ("cmd \"\"", result);
    free(result);
}

/* Test: Mixed quoted and unquoted */
void test_mixed_args(void)
{
    char *argv[] = { "ssh", "-o", "StrictHostKeyChecking no", "user@host" };
    char *result = build_command_line(4, argv);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ("ssh -o \"StrictHostKeyChecking no\" user@host", result);
    free(result);
}

/* Test: Path with spaces */
void test_path_with_spaces(void)
{
    char *argv[] = { "C:\\Program Files\\OpenSSH\\ssh.exe", "user@host" };
    char *result = build_command_line(2, argv);
    ASSERT_NOT_NULL(result);
    ASSERT_STR_EQ("\"C:\\Program Files\\OpenSSH\\ssh.exe\" user@host", result);
    free(result);
}

int main(void)
{
    fprintf(stderr, "=== build_command_line() unit tests ===\n");

    RUN_TEST(test_single_arg);
    RUN_TEST(test_multiple_args);
    RUN_TEST(test_arg_with_spaces);
    RUN_TEST(test_arg_with_tab);
    RUN_TEST(test_empty_arg);
    RUN_TEST(test_mixed_args);
    RUN_TEST(test_path_with_spaces);

    TEST_SUMMARY();
    return test_failures;
}
