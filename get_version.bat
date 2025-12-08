@echo off
setlocal enabledelayedexpansion

rem --- Check if a file was provided ---
if "%~1"=="" (
    echo Usage: get_version.bat path\to\cobgdb.c
    exit /b 1
)

rem --- Check if the file exists ---
if not exist "%~1" (
    echo File not found: %~1
    exit /b 1
)

rem --- Read the line containing #define COBGDB_VERSION ---
for /f "tokens=3 delims= " %%a in ('type "%~1" ^| findstr /R /C:"#define COBGDB_VERSION"') do (
    set "ver=%%~a"
)

rem --- Remove quotes and display ---
if defined ver (
    set "ver=!ver:"=!"
    echo !ver!
) else (
    echo 0.0.0
)

endlocal
exit /b 0
