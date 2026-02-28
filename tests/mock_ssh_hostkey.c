/*
 * Mock SSH program that simulates host key warnings for integration testing
 *
 * Modes (controlled by argv[1]):
 *   "unknown"  - prints "The authenticity of host ..." message
 *   "changed"  - prints host key changed warning with IP address mismatch
 *
 * Usage: mock_ssh_hostkey.exe [unknown|changed]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    const char *mode = "unknown";
    if (argc > 1) {
        mode = argv[1];
    }

    if (strcmp(mode, "changed") == 0) {
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
        printf("@    WARNING: REMOTE HOST IDENTIFICATION HAS CHANGED!     @\r\n");
        printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\r\n");
        printf("IT IS POSSIBLE THAT SOMEONE IS DOING SOMETHING NASTY!\r\n");
        printf("Host key for server differs from the key for the IP address '10.0.0.1'\r\n");
        fflush(stdout);
        return 1;
    } else {
        /* Default: unknown host key */
        printf("The authenticity of host 'example.com (93.184.216.34)' can't be established.\r\n");
        printf("ED25519 key fingerprint is SHA256:abcdefghijklmnopqrstuvwxyz.\r\n");
        printf("Are you sure you want to continue connecting (yes/no/[fingerprint])? ");
        fflush(stdout);

        /* Wait for input (sshpass should kill us before we get here) */
        char buf[64];
        if (fgets(buf, sizeof(buf), stdin) != NULL) {
            /* If somehow we get input, just exit */
        }
        return 1;
    }
}
