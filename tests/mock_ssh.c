/*
 * Mock SSH program for integration testing sshpass
 *
 * Simulates an SSH password prompt. When running under ConPTY,
 * stdin/stdout are pseudo-console handles (FILE_TYPE_CHAR) but
 * Console API calls fail - must use ReadFile/WriteFile.
 *
 * Usage: mock_ssh.exe [expected_password]
 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void write_str(HANDLE h, const char *s)
{
    DWORD written;
    WriteFile(h, s, (DWORD)strlen(s), &written, NULL);
}

static int read_line(HANDLE h, char *buf, int bufsize)
{
    int pos = 0;
    int retries = 0;

    while (pos < bufsize - 1) {
        char ch;
        DWORD bytesRead = 0;
        BOOL ok = ReadFile(h, &ch, 1, &bytesRead, NULL);

        if (!ok) break;

        if (bytesRead == 0) {
            /* No data yet - ConPTY may need time to deliver input */
            if (retries++ > 200) break;  /* ~2 seconds max wait */
            Sleep(10);
            continue;
        }

        retries = 0;  /* Reset retry counter on successful read */

        if (ch == '\r' || ch == '\n') {
            /* Consume the paired \n after \r if present */
            if (ch == '\r') {
                DWORD br2;
                char ch2;
                /* Peek to see if \n follows */
                Sleep(10);
                ReadFile(h, &ch2, 1, &br2, NULL);
                /* Don't care about result - just consume it */
            }
            break;
        }

        buf[pos++] = ch;
    }
    buf[pos] = '\0';
    return pos;
}

int main(int argc, char *argv[])
{
    const char *expected = "correctpassword";
    if (argc > 1) {
        expected = argv[1];
    }

    HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    char input[256];
    int attempts = 0;
    int max_attempts = 2;

    while (attempts < max_attempts) {
        /* Write password prompt (must contain "assword" for sshpass to detect) */
        write_str(hStdout, "user@host's password: ");

        /* Read password */
        int len = read_line(hStdin, input, sizeof(input));

        if (len > 0 && strcmp(input, expected) == 0) {
            write_str(hStdout, "\r\nWelcome to mock SSH server\r\n");
            return 0;
        }

        write_str(hStdout, "\r\nPermission denied, please try again.\r\n");
        attempts++;
    }

    write_str(hStdout, "Permission denied (publickey,password).\r\n");
    return 1;
}
