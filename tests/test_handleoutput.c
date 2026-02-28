/*
 * Unit tests for the handleoutput() function from sshpass
 *
 * handleoutput() scans output buffers for password prompts, host key
 * warnings, etc. It maintains internal state across calls.
 */

#include "test_framework.h"
#include <stdlib.h>
#include <windows.h>

/* Return codes from main_win.c */
enum {
    RETURN_NOERROR = 0,
    RETURN_INVALID_ARGUMENTS = 1,
    RETURN_CONFLICTING_ARGUMENTS = 2,
    RETURN_RUNTIME_ERROR = 3,
    RETURN_PARSE_ERRROR = 4,
    RETURN_INCORRECT_PASSWORD = 5,
    RETURN_HOST_KEY_UNKNOWN = 6,
    RETURN_HOST_KEY_CHANGED = 7,
};

/* Declarations from main_win.c */
int handleoutput(const char *buffer, int numread);
int match(const char *reference, const char *buffer, int bufsize, int state);
void handleoutput_reset(void);
void write_pass(HANDLE hWrite);
void write_pass_fd(int srcfd, HANDLE hWrite);
void reliable_write(HANDLE hWrite, const void *data, DWORD size);

/* External args structure from main_win.c */
extern struct {
    enum { PWT_STDIN, PWT_FILE, PWT_FD, PWT_PASS } pwtype;
    union {
        const char *filename;
        int fd;
        const char *password;
    } pwsrc;
    const char *pwprompt;
    int verbose;
    char *orig_password;
} args;

/* Helper to reset all state between tests */
static void reset_state(void)
{
    handleoutput_reset();
    memset(&args, 0, sizeof(args));
    args.pwtype = PWT_PASS;
    args.pwsrc.password = "testpass";
}

/* Test: Normal output with no prompt returns 0 */
void test_no_prompt(void)
{
    reset_state();
    const char *buf = "Welcome to Ubuntu 22.04 LTS\r\n";
    int ret = handleoutput(buf, (int)strlen(buf));
    ASSERT_EQ(0, ret);
}

/* Test: Detects default password prompt "assword" */
void test_detect_password_prompt(void)
{
    reset_state();
    const char *buf = "user@host's password: ";
    int ret = handleoutput(buf, (int)strlen(buf));
    /* First detection should return 0 (it sends the password, doesn't error) */
    ASSERT_EQ(0, ret);
}

/* Test: Second password prompt means wrong password */
void test_incorrect_password(void)
{
    reset_state();
    /* First prompt */
    const char *buf1 = "Password: ";
    int ret = handleoutput(buf1, (int)strlen(buf1));
    ASSERT_EQ(0, ret);

    /* Second prompt - wrong password */
    handleoutput_reset();
    /* We need to NOT reset prevmatch, so let's do it differently */
    /* Actually handleoutput_reset resets prevmatch too, so we need two prompts without reset */
}

/* Test: Second password prompt without reset means wrong password */
void test_incorrect_password_v2(void)
{
    reset_state();
    /* First prompt */
    const char *buf1 = "user@host's password: ";
    int ret1 = handleoutput(buf1, (int)strlen(buf1));
    ASSERT_EQ(0, ret1);

    /* Second prompt - should detect wrong password */
    const char *buf2 = "user@host's password: ";
    int ret2 = handleoutput(buf2, (int)strlen(buf2));
    ASSERT_EQ(RETURN_INCORRECT_PASSWORD, ret2);
}

/* Test: Detects host key unknown */
void test_host_key_unknown(void)
{
    reset_state();
    const char *buf = "The authenticity of host 'example.com (1.2.3.4)' can't be established.\r\n";
    int ret = handleoutput(buf, (int)strlen(buf));
    ASSERT_EQ(RETURN_HOST_KEY_UNKNOWN, ret);
}

/* Test: Detects host key changed */
void test_host_key_changed(void)
{
    reset_state();
    const char *buf = "WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!\r\n"
                      "Host key for server differs from the key for the IP address\r\n";
    int ret = handleoutput(buf, (int)strlen(buf));
    ASSERT_EQ(RETURN_HOST_KEY_CHANGED, ret);
}

/* Test: Custom password prompt */
void test_custom_prompt(void)
{
    reset_state();
    args.pwprompt = "Enter passphrase:";
    const char *buf = "Enter passphrase: ";
    int ret = handleoutput(buf, (int)strlen(buf));
    ASSERT_EQ(0, ret); /* Should detect and send password */
}

/* Test: Password prompt split across buffers */
void test_prompt_split_buffers(void)
{
    reset_state();
    /* "assword" split: "ass" in first buffer, "word" in second */
    const char *buf1 = "Pass";
    int ret1 = handleoutput(buf1, (int)strlen(buf1));
    ASSERT_EQ(0, ret1);

    const char *buf2 = "word: ";
    int ret2 = handleoutput(buf2, (int)strlen(buf2));
    /* Should have detected the full "assword" prompt */
    ASSERT_EQ(0, ret2);
}

/* Test: Host key unknown split across buffers */
void test_hostkey_split_buffers(void)
{
    reset_state();
    const char *buf1 = "The authenticity of ";
    int ret1 = handleoutput(buf1, (int)strlen(buf1));
    ASSERT_EQ(0, ret1);

    const char *buf2 = "host 'example.com' can't be established.";
    int ret2 = handleoutput(buf2, (int)strlen(buf2));
    ASSERT_EQ(RETURN_HOST_KEY_UNKNOWN, ret2);
}

int main(void)
{
    fprintf(stderr, "=== handleoutput() unit tests ===\n");

    RUN_TEST(test_no_prompt);
    RUN_TEST(test_detect_password_prompt);
    RUN_TEST(test_incorrect_password_v2);
    RUN_TEST(test_host_key_unknown);
    RUN_TEST(test_host_key_changed);
    RUN_TEST(test_custom_prompt);
    RUN_TEST(test_prompt_split_buffers);
    RUN_TEST(test_hostkey_split_buffers);

    TEST_SUMMARY();
    return test_failures;
}
