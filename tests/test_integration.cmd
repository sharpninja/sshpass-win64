@echo off
setlocal enabledelayedexpansion

:: Integration tests for sshpass-win64
:: Tests CLI argument parsing, help/version output, and error handling.
::
:: Usage: test_integration.cmd [build_dir]

set BUILD_DIR=%~1
if "%BUILD_DIR%"=="" set BUILD_DIR=.

set SSHPASS=%BUILD_DIR%\sshpass.exe

set PASS=0
set FAIL=0

:: Verify binary exists
if not exist "%SSHPASS%" (
    echo ERROR: sshpass.exe not found at %SSHPASS%
    exit /b 1
)

echo === Integration Tests ===
echo   sshpass: %SSHPASS%
echo.

:: --- Test: Help flag returns 0 ---
%SSHPASS% -h >nul 2>&1
set EC=!ERRORLEVEL!
if "!EC!"=="0" ( echo   PASS: help_flag & set /a PASS+=1 ) else ( echo   FAIL: help_flag ^(expected 0, got !EC!^) & set /a FAIL+=1 )

:: --- Test: No arguments returns 0 (shows help) ---
%SSHPASS% >nul 2>&1
set EC=!ERRORLEVEL!
if "!EC!"=="0" ( echo   PASS: no_args & set /a PASS+=1 ) else ( echo   FAIL: no_args ^(expected 0, got !EC!^) & set /a FAIL+=1 )

:: --- Test: Version flag returns 0 ---
%SSHPASS% -V >nul 2>&1
set EC=!ERRORLEVEL!
if "!EC!"=="0" ( echo   PASS: version_flag & set /a PASS+=1 ) else ( echo   FAIL: version_flag ^(expected 0, got !EC!^) & set /a FAIL+=1 )

:: --- Test: Conflicting options -p and -f returns 2 ---
%SSHPASS% -p pass1 -f file.txt echo test >nul 2>&1
set EC=!ERRORLEVEL!
if "!EC!"=="2" ( echo   PASS: conflicting_p_f & set /a PASS+=1 ) else ( echo   FAIL: conflicting_p_f ^(expected 2, got !EC!^) & set /a FAIL+=1 )

:: --- Test: -p with nonexistent command returns 3 (runtime error) ---
%SSHPASS% -p test nonexistent_command_12345 >nul 2>&1
set EC=!ERRORLEVEL!
if "!EC!"=="3" ( echo   PASS: nonexistent_command & set /a PASS+=1 ) else ( echo   FAIL: nonexistent_command ^(expected 3, got !EC!^) & set /a FAIL+=1 )

:: --- Test: -h output contains usage text ---
%SSHPASS% -h 2>&1 | findstr /C:"Usage:" >nul 2>&1
set EC=!ERRORLEVEL!
if "!EC!"=="0" ( echo   PASS: help_contains_usage & set /a PASS+=1 ) else ( echo   FAIL: help_contains_usage ^(expected 0, got !EC!^) & set /a FAIL+=1 )

:: --- Test: -V output contains version string ---
%SSHPASS% -V 2>&1 | findstr /C:"sshpass 1.10" >nul 2>&1
set EC=!ERRORLEVEL!
if "!EC!"=="0" ( echo   PASS: version_contains_string & set /a PASS+=1 ) else ( echo   FAIL: version_contains_string ^(expected 0, got !EC!^) & set /a FAIL+=1 )

:: --- Test: Verbose flag with help still works ---
%SSHPASS% -v -h >nul 2>&1
set EC=!ERRORLEVEL!
if "!EC!"=="0" ( echo   PASS: verbose_help & set /a PASS+=1 ) else ( echo   FAIL: verbose_help ^(expected 0, got !EC!^) & set /a FAIL+=1 )

:: --- Summary ---
echo.
set /a TOTAL=PASS+FAIL
echo !PASS!/!TOTAL! tests passed
if !FAIL! GTR 0 (
    echo !FAIL! FAILED
    exit /b 1
) else (
    echo ^(all passed^)
    exit /b 0
)
