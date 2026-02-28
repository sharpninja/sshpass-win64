/*  This file is part of "sshpass", a tool for batch running password ssh authentication
 *  Copyright (C) 2006 Lingnu Open Source Consulting Ltd.
 *  Copyright (C) 2015-2016, 2021 Shachar Shemesh
 *
 *  Windows native port using ConPTY API
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version, provided that it was accepted by
 *  Lingnu Open Source Consulting Ltd. as an acceptable license for its
 *  projects. Consult http://www.lingnu.com/licenses.html
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#define _WIN32_WINNT 0x0A00  /* Windows 10 */
#define NTDDI_VERSION 0x0A000006  /* Windows 10 1809 */

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <io.h>
#include <assert.h>
#include <errno.h>

/* getopt support - MinGW provides this */
#include <getopt.h>

#define PACKAGE_NAME "sshpass"
#define PACKAGE_STRING "sshpass 1.10"
#define PASSWORD_PROMPT "assword"

enum program_return_codes {
    RETURN_NOERROR,
    RETURN_INVALID_ARGUMENTS,
    RETURN_CONFLICTING_ARGUMENTS,
    RETURN_RUNTIME_ERROR,
    RETURN_PARSE_ERRROR,
    RETURN_INCORRECT_PASSWORD,
    RETURN_HOST_KEY_UNKNOWN,
    RETURN_HOST_KEY_CHANGED,
    RETURN_HELP,
};

/* Forward declarations */
int runprogram(int argc, char *argv[]);
int handleoutput(const char *buffer, int numread);
int match(const char *reference, const char *buffer, int bufsize, int state);
void write_pass(HANDLE hWrite);
void write_pass_fd(int srcfd, HANDLE hWrite);
void reliable_write(HANDLE hWrite, const void *data, DWORD size);

/* Global args structure */
struct {
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

static void hide_password()
{
    assert(args.pwsrc.password == NULL);

    args.pwsrc.password = _strdup(args.orig_password);

    /* Hide the original password from prying eyes */
    while (*args.orig_password != '\0') {
        *args.orig_password = '\0';
        ++args.orig_password;
    }

    args.orig_password = NULL;
}

static void show_help()
{
    printf("Usage: " PACKAGE_NAME " [-f|-d|-p|-e[env_var]] [-hV] command parameters\n"
            "   -f filename   Take password to use from file.\n"
            "   -d number     Use number as file descriptor for getting password.\n"
            "   -p password   Provide password as argument (security unwise).\n"
            "   -e[env_var]   Password is passed as env-var \"env_var\" if given, \"SSHPASS\" otherwise.\n"
            "   With no parameters - password will be taken from stdin.\n\n"
            "   -P prompt     Which string should sshpass search for to detect a password prompt.\n"
            "   -v            Be verbose about what you're doing.\n"
            "   -h            Show help (this screen).\n"
            "   -V            Print version information.\n"
            "At most one of -f, -d, -p or -e should be used.\n");
}

/* Parse the command line. Fill in the "args" global struct with the results. Return argv offset
   on success, and a negative number on failure */
static int parse_options(int argc, char *argv[])
{
    int error = -1;
    int opt;

    /* Set the default password source to stdin */
    args.pwtype = PWT_STDIN;
    args.pwsrc.fd = 0;

#define VIRGIN_PWTYPE if (args.pwtype != PWT_STDIN) { \
    fprintf(stderr, "Conflicting password source\n"); \
    error = RETURN_CONFLICTING_ARGUMENTS; }

    while ((opt = getopt(argc, argv, "+f:d:p:P:he::Vv")) != -1 && error == -1) {
        switch (opt) {
        case 'f':
            /* Password should come from a file */
            VIRGIN_PWTYPE;
            args.pwtype = PWT_FILE;
            args.pwsrc.filename = optarg;
            break;
        case 'd':
            /* Password should come from an open file descriptor */
            VIRGIN_PWTYPE;
            args.pwtype = PWT_FD;
            args.pwsrc.fd = atoi(optarg);
            break;
        case 'p':
            /* Password is given on the command line */
            VIRGIN_PWTYPE;
            args.pwtype = PWT_PASS;
            args.orig_password = optarg;
            break;
        case 'P':
            args.pwprompt = optarg;
            break;
        case 'v':
            args.verbose++;
            break;
        case 'e':
            VIRGIN_PWTYPE;
            args.pwtype = PWT_PASS;
            if (optarg == NULL)
                optarg = "SSHPASS";
            args.orig_password = getenv(optarg);

            if (args.orig_password == NULL) {
                fprintf(stderr, "sshpass: -e option given but \"%s\" environment variable is not set.\n", optarg);
                error = RETURN_INVALID_ARGUMENTS;
            }

            hide_password();
            _putenv_s(optarg, "");
            break;
        case '?':
        case ':':
            error = RETURN_INVALID_ARGUMENTS;
            break;
        case 'h':
            return -(RETURN_HELP + 1);
        case 'V':
            printf("%s\n"
                    "(C) 2006-2011 Lingnu Open Source Consulting Ltd.\n"
                    "(C) 2015-2016, 2021-2022 Shachar Shemesh\n"
                    "This program is free software, and can be distributed under the terms of the GPL\n"
                    "See the COPYING file for more information.\n"
                    "\n"
                    "Using \"%s\" as the default password prompt indicator.\n", PACKAGE_STRING, PASSWORD_PROMPT);
            exit(0);
            break;
        }
    }

    if (error >= 0)
        return -(error + 1);
    else
        return optind;
}

int main(int argc, char *argv[])
{
    int opt_offset = parse_options(argc, argv);

    if (opt_offset == -(RETURN_HELP + 1)) {
        show_help();
        return 0;
    }

    if (opt_offset < 0) {
        fprintf(stderr, "Use \"sshpass -h\" to get help\n");
        return -(opt_offset + 1);
    }

    if (argc - opt_offset < 1) {
        show_help();
        return 0;
    }

    if (args.orig_password != NULL) {
        hide_password();
    }

    return runprogram(argc - opt_offset, argv + opt_offset);
}

/*
 * Build a command line string from argv for CreateProcess.
 * Each argument is quoted if it contains spaces or special characters.
 */
static char *build_command_line(int argc, char *argv[])
{
    /* Calculate total length needed */
    size_t total = 0;
    int i;
    for (i = 0; i < argc; i++) {
        total += strlen(argv[i]) + 3; /* quotes + space */
    }
    total += 1; /* null terminator */

    char *cmdline = (char *)malloc(total);
    if (!cmdline) return NULL;

    cmdline[0] = '\0';
    for (i = 0; i < argc; i++) {
        if (i > 0) strcat(cmdline, " ");

        /* Quote arguments that contain spaces, or always quote for safety */
        int needs_quote = (strchr(argv[i], ' ') != NULL || strchr(argv[i], '\t') != NULL || argv[i][0] == '\0');
        if (needs_quote) strcat(cmdline, "\"");
        strcat(cmdline, argv[i]);
        if (needs_quote) strcat(cmdline, "\"");
    }

    return cmdline;
}

/* Global handle for writing to the ConPTY - set in runprogram() */
static HANDLE g_hPipeIn = INVALID_HANDLE_VALUE;

/*
 * Dynamically load ConPTY functions to support compilation on older SDKs
 * while still running on Windows 10 1809+
 */
typedef HRESULT (WINAPI *PFN_CreatePseudoConsole)(COORD size, HANDLE hInput, HANDLE hOutput, DWORD dwFlags, HPCON *phPC);
typedef void (WINAPI *PFN_ClosePseudoConsole)(HPCON hPC);
typedef HRESULT (WINAPI *PFN_ResizePseudoConsole)(HPCON hPC, COORD size);

static PFN_CreatePseudoConsole pCreatePseudoConsole = NULL;
static PFN_ClosePseudoConsole pClosePseudoConsole = NULL;
static PFN_ResizePseudoConsole pResizePseudoConsole = NULL;

static int load_conpty_api(void)
{
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
    if (!hKernel32) return 0;

    pCreatePseudoConsole = (PFN_CreatePseudoConsole)GetProcAddress(hKernel32, "CreatePseudoConsole");
    pClosePseudoConsole = (PFN_ClosePseudoConsole)GetProcAddress(hKernel32, "ClosePseudoConsole");
    pResizePseudoConsole = (PFN_ResizePseudoConsole)GetProcAddress(hKernel32, "ResizePseudoConsole");

    return (pCreatePseudoConsole && pClosePseudoConsole && pResizePseudoConsole);
}

/*
 * Initialize a STARTUPINFOEX with the pseudo console attribute
 */
static BOOL setup_startup_info(STARTUPINFOEXA *psi, HPCON hPC)
{
    SIZE_T attrListSize = 0;

    ZeroMemory(psi, sizeof(*psi));
    psi->StartupInfo.cb = sizeof(STARTUPINFOEXA);

    /* Get required size */
    InitializeProcThreadAttributeList(NULL, 1, 0, &attrListSize);

    psi->lpAttributeList = (LPPROC_THREAD_ATTRIBUTE_LIST)HeapAlloc(
        GetProcessHeap(), 0, attrListSize);
    if (!psi->lpAttributeList) return FALSE;

    if (!InitializeProcThreadAttributeList(psi->lpAttributeList, 1, 0, &attrListSize)) {
        HeapFree(GetProcessHeap(), 0, psi->lpAttributeList);
        return FALSE;
    }

    if (!UpdateProcThreadAttribute(
            psi->lpAttributeList,
            0,
            PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE,
            hPC,
            sizeof(HPCON),
            NULL,
            NULL)) {
        DeleteProcThreadAttributeList(psi->lpAttributeList);
        HeapFree(GetProcessHeap(), 0, psi->lpAttributeList);
        return FALSE;
    }

    return TRUE;
}

static void cleanup_startup_info(STARTUPINFOEXA *psi)
{
    if (psi->lpAttributeList) {
        DeleteProcThreadAttributeList(psi->lpAttributeList);
        HeapFree(GetProcessHeap(), 0, psi->lpAttributeList);
        psi->lpAttributeList = NULL;
    }
}

int runprogram(int argc, char *argv[])
{
    HRESULT hr;
    HPCON hPC = NULL;
    HANDLE hPipeIn = INVALID_HANDLE_VALUE;   /* Pipe: ConPTY reads from this (we write to it) */
    HANDLE hPipeOut = INVALID_HANDLE_VALUE;  /* Pipe: ConPTY writes to this (we read from it) */
    HANDLE hPipePTYIn = INVALID_HANDLE_VALUE;
    HANDLE hPipePTYOut = INVALID_HANDLE_VALUE;
    PROCESS_INFORMATION pi;
    STARTUPINFOEXA si;
    COORD consoleSize;
    char *cmdline = NULL;
    int result = RETURN_RUNTIME_ERROR;

    ZeroMemory(&pi, sizeof(pi));
    ZeroMemory(&si, sizeof(si));

    /* Load ConPTY API */
    if (!load_conpty_api()) {
        fprintf(stderr, "SSHPASS: ConPTY API not available. Windows 10 version 1809 or later is required.\n");
        return RETURN_RUNTIME_ERROR;
    }

    /* Create pipes for the pseudo console */
    /* hPipeIn -> hPipePTYIn: we write to hPipeIn, ConPTY reads from hPipePTYIn */
    /* hPipePTYOut -> hPipeOut: ConPTY writes to hPipePTYOut, we read from hPipeOut */
    if (!CreatePipe(&hPipePTYIn, &hPipeIn, NULL, 0)) {
        fprintf(stderr, "SSHPASS: Failed to create input pipe: %lu\n", GetLastError());
        return RETURN_RUNTIME_ERROR;
    }
    if (!CreatePipe(&hPipeOut, &hPipePTYOut, NULL, 0)) {
        fprintf(stderr, "SSHPASS: Failed to create output pipe: %lu\n", GetLastError());
        CloseHandle(hPipePTYIn);
        CloseHandle(hPipeIn);
        return RETURN_RUNTIME_ERROR;
    }

    /* Get console size */
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hConsole != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hConsole, &csbi)) {
        consoleSize.X = csbi.srWindow.Right - csbi.srWindow.Left + 1;
        consoleSize.Y = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    } else {
        consoleSize.X = 80;
        consoleSize.Y = 24;
    }

    /* Create the pseudo console */
    hr = pCreatePseudoConsole(consoleSize, hPipePTYIn, hPipePTYOut, 0, &hPC);
    if (FAILED(hr)) {
        fprintf(stderr, "SSHPASS: Failed to create pseudo console: 0x%08lx\n", (unsigned long)hr);
        goto cleanup;
    }

    /* Close the PTY-side pipe handles (now owned by the pseudo console) */
    CloseHandle(hPipePTYIn);
    hPipePTYIn = INVALID_HANDLE_VALUE;
    CloseHandle(hPipePTYOut);
    hPipePTYOut = INVALID_HANDLE_VALUE;

    /* Setup startup info with pseudo console */
    if (!setup_startup_info(&si, hPC)) {
        fprintf(stderr, "SSHPASS: Failed to setup startup info: %lu\n", GetLastError());
        goto cleanup;
    }

    /* Build command line */
    cmdline = build_command_line(argc, argv);
    if (!cmdline) {
        fprintf(stderr, "SSHPASS: Failed to allocate command line\n");
        goto cleanup;
    }

    if (args.verbose) {
        fprintf(stderr, "SSHPASS: launching command: %s\n", cmdline);
    }

    /* Create the child process */
    if (!CreateProcessA(
            NULL,           /* Application name (use command line) */
            cmdline,        /* Command line */
            NULL,           /* Process security attributes */
            NULL,           /* Thread security attributes */
            FALSE,          /* Inherit handles */
            EXTENDED_STARTUPINFO_PRESENT,  /* Creation flags */
            NULL,           /* Environment (inherit) */
            NULL,           /* Current directory (inherit) */
            &si.StartupInfo,/* Startup info */
            &pi))           /* Process information */
    {
        fprintf(stderr, "SSHPASS: Failed to create process '%s': %lu\n", cmdline, GetLastError());
        goto cleanup;
    }

    /* Set global pipe handle so write_pass() can use it */
    g_hPipeIn = hPipeIn;

    /* Main loop: read from ConPTY output, look for password prompt */
    {
        char buffer[256];
        DWORD bytesRead;
        int terminate = 0;
        DWORD exitCode;

        while (!terminate) {
            /* Check if process has exited */
            DWORD waitResult = WaitForSingleObject(pi.hProcess, 0);

            /* Try to read from the pipe */
            BOOL readOk = ReadFile(hPipeOut, buffer, sizeof(buffer) - 1, &bytesRead, NULL);

            if (readOk && bytesRead > 0) {
                buffer[bytesRead] = '\0';

                if (args.verbose) {
                    fprintf(stderr, "SSHPASS: read: %s\n", buffer);
                }

                int ret = handleoutput(buffer, (int)bytesRead);
                if (ret) {
                    if (ret > 0) {
                        /* Authentication error - close our write end to signal the process */
                        CloseHandle(hPipeIn);
                        hPipeIn = INVALID_HANDLE_VALUE;
                        g_hPipeIn = INVALID_HANDLE_VALUE;
                    }
                    terminate = ret;
                }
            } else {
                /* Read failed or zero bytes - pipe closed or process exited */
                if (waitResult == WAIT_OBJECT_0) {
                    /* Process has exited */
                    break;
                }
                /* Small sleep to avoid busy-waiting if read returned 0 */
                if (!readOk) break;
                Sleep(10);
            }

            /* Check again if process exited */
            if (waitResult == WAIT_OBJECT_0) {
                /* Drain remaining output */
                while (ReadFile(hPipeOut, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
                    buffer[bytesRead] = '\0';
                    if (args.verbose) {
                        fprintf(stderr, "SSHPASS: read (drain): %s\n", buffer);
                    }
                    int ret = handleoutput(buffer, (int)bytesRead);
                    if (ret && !terminate) {
                        terminate = ret;
                    }
                }
                break;
            }
        }

        /* Wait for process to fully exit */
        WaitForSingleObject(pi.hProcess, INFINITE);
        GetExitCodeProcess(pi.hProcess, &exitCode);

        if (terminate > 0)
            result = terminate;
        else
            result = (int)exitCode;
    }

cleanup:
    if (cmdline) free(cmdline);
    cleanup_startup_info(&si);
    if (pi.hProcess) CloseHandle(pi.hProcess);
    if (pi.hThread) CloseHandle(pi.hThread);
    if (hPC) pClosePseudoConsole(hPC);
    if (hPipeIn != INVALID_HANDLE_VALUE) CloseHandle(hPipeIn);
    if (hPipeOut != INVALID_HANDLE_VALUE) CloseHandle(hPipeOut);
    if (hPipePTYIn != INVALID_HANDLE_VALUE) CloseHandle(hPipePTYIn);
    if (hPipePTYOut != INVALID_HANDLE_VALUE) CloseHandle(hPipePTYOut);

    return result;
}

int handleoutput(const char *buffer, int numread)
{
    static int prevmatch = 0;
    static int state1, state2, state3;
    static int firsttime = 1;
    static const char *compare1 = PASSWORD_PROMPT;
    static const char compare2[] = "The authenticity of host ";
    static const char compare3[] = "differs from the key for the IP address";
    int ret = 0;

    if (args.pwprompt) {
        compare1 = args.pwprompt;
    }

    if (args.verbose && firsttime) {
        firsttime = 0;
        fprintf(stderr, "SSHPASS: searching for password prompt using match \"%s\"\n", compare1);
    }

    state1 = match(compare1, buffer, numread, state1);

    /* Are we at a password prompt? */
    if (compare1[state1] == '\0') {
        if (!prevmatch) {
            if (args.verbose)
                fprintf(stderr, "SSHPASS: detected prompt. Sending password.\n");
            write_pass(INVALID_HANDLE_VALUE); /* Will use global pipe handle */
            state1 = 0;
            prevmatch = 1;
        } else {
            if (args.verbose)
                fprintf(stderr, "SSHPASS: detected prompt, again. Wrong password. Terminating.\n");
            ret = RETURN_INCORRECT_PASSWORD;
        }
    }

    if (ret == 0) {
        state2 = match(compare2, buffer, numread, state2);

        if (compare2[state2] == '\0') {
            if (args.verbose)
                fprintf(stderr, "SSHPASS: detected host authentication prompt. Exiting.\n");
            ret = RETURN_HOST_KEY_UNKNOWN;
        } else {
            state3 = match(compare3, buffer, numread, state3);
            if (compare3[state3] == '\0') {
                ret = RETURN_HOST_KEY_CHANGED;
            }
        }
    }

    return ret;
}

int match(const char *reference, const char *buffer, int bufsize, int state)
{
    int i;
    for (i = 0; reference[state] != '\0' && i < bufsize; ++i) {
        if (reference[state] == buffer[i])
            state++;
        else {
            state = 0;
            if (reference[state] == buffer[i])
                state++;
        }
    }

    return state;
}

void write_pass(HANDLE hWrite)
{
    /* Use the global pipe handle */
    HANDLE fd = g_hPipeIn;
    if (fd == INVALID_HANDLE_VALUE) return;

    switch (args.pwtype) {
    case PWT_STDIN:
        write_pass_fd(0, fd); /* stdin = fd 0 */
        break;
    case PWT_FD:
        write_pass_fd(args.pwsrc.fd, fd);
        break;
    case PWT_FILE:
        {
            int srcfd = _open(args.pwsrc.filename, _O_RDONLY);
            if (srcfd != -1) {
                write_pass_fd(srcfd, fd);
                _close(srcfd);
            } else {
                fprintf(stderr, "SSHPASS: Failed to open password file \"%s\": %s\n",
                        args.pwsrc.filename, strerror(errno));
            }
        }
        break;
    case PWT_PASS:
        reliable_write(fd, args.pwsrc.password, (DWORD)strlen(args.pwsrc.password));
        reliable_write(fd, "\r\n", 2);
        break;
    }
}

void write_pass_fd(int srcfd, HANDLE dstHandle)
{
    int done = 0;

    while (!done) {
        char buffer[40];
        int i;
        int numread = _read(srcfd, buffer, sizeof(buffer));
        done = (numread < 1);
        for (i = 0; i < numread && !done; ++i) {
            if (buffer[i] != '\n' && buffer[i] != '\r')
                reliable_write(dstHandle, buffer + i, 1);
            else
                done = 1;
        }
    }

    reliable_write(dstHandle, "\r\n", 2);
}

void reliable_write(HANDLE hWrite, const void *data, DWORD size)
{
    DWORD written;
    BOOL result = WriteFile(hWrite, data, size, &written, NULL);
    if (!result || written != size) {
        if (!result) {
            fprintf(stderr, "SSHPASS: write failed: %lu\n", GetLastError());
        } else {
            fprintf(stderr, "SSHPASS: Short write. Tried to write %lu, only wrote %lu\n",
                    (unsigned long)size, (unsigned long)written);
        }
    }
}
