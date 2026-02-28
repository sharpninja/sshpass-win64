# sshpass — Native Win64 Port

This is a **native Windows 64-bit port** of [sshpass](https://sourceforge.net/projects/sshpass/) 1.10, the non-interactive SSH password authentication tool.

## Overview

`sshpass` is a utility designed for running SSH in non-interactive mode, providing the password via the command line, environment variable, or file. The original Linux version uses PTY (pseudo-terminal) and `fork()`/`exec()` to intercept the SSH password prompt.

This port replaces the Linux-specific PTY/fork mechanism with the **Windows ConPTY (Pseudo Console) API**, introduced in Windows 10 version 1809 (October 2018 Update). The result is a fully native Win64 binary with no Cygwin or WSL dependency at runtime.

## Requirements

- **Runtime:** Windows 10 version 1809 or later (for ConPTY API support)
- **Build:** MinGW-w64 (GCC for Windows) with `getopt.h` support
- **SSH client:** OpenSSH for Windows (ships with Windows 10 1803+), or any other SSH client

## Building

```bash
make -f Makefile.win
```

This produces `sshpass.exe` using MinGW-w64 GCC.

To clean build artifacts:

```bash
make -f Makefile.win clean
```

## Usage

The command-line interface is identical to the original sshpass:

```
Usage: sshpass [-f|-d|-p|-e[env_var]] [-hV] command parameters
   -f filename   Take password to use from file.
   -d number     Use number as file descriptor for getting password.
   -p password   Provide password as argument (security unwise).
   -e[env_var]   Password is passed as env-var "env_var" if given, "SSHPASS" otherwise.
   With no parameters - password will be taken from stdin.

   -P prompt     Which string should sshpass search for to detect a password prompt.
   -v            Be verbose about what you're doing.
   -h            Show help (this screen).
   -V            Print version information.
At most one of -f, -d, -p or -e should be used.
```

### Examples

```bash
# Using a password directly (not recommended for production)
sshpass -p "mypassword" ssh user@host

# Using an environment variable
set SSHPASS=mypassword
sshpass -e ssh user@host

# Using a password file
sshpass -f password.txt ssh user@host

# SCP with password
sshpass -p "mypassword" scp file.txt user@host:/path/
```

## How It Works

Instead of the Linux PTY approach, this port:

1. Creates a **Windows Pseudo Console (ConPTY)** via `CreatePseudoConsole()`
2. Launches the child process (e.g., `ssh`) attached to the pseudo console
3. Monitors the console output for the password prompt (default: `"assword"`)
4. Sends the password through the pseudo console input pipe when the prompt is detected
5. Forwards the child process exit code

The ConPTY API is loaded dynamically at runtime, so the binary can be compiled on older SDKs while still requiring Windows 10 1809+ to run.

## Key Differences from the Linux Version

| Feature | Linux | Windows (this port) |
|---|---|---|
| Terminal emulation | PTY (`posix_openpt`) | ConPTY (`CreatePseudoConsole`) |
| Process creation | `fork()` + `exec()` | `CreateProcessA()` with `EXTENDED_STARTUPINFO_PRESENT` |
| Line ending | `\n` | `\r\n` |
| Source file | `main.c` | `main_win.c` |
| Build system | autotools (`./configure && make`) | `make -f Makefile.win` (MinGW) |

## License

This program is free software, distributed under the terms of the **GNU General Public License v2** (or later, as accepted by Lingnu Open Source Consulting Ltd.). See the [COPYING](COPYING) file for details.

## Credits

- **Original sshpass:** Copyright © 2006-2022 Lingnu Open Source Consulting Ltd. / Shachar Shemesh
- **Windows ConPTY port:** Native Win64 adaptation
