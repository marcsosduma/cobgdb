@echo off
setlocal enabledelayedexpansion

rem --- Verifica se foi passado um arquivo ---
if "%~1"=="" (
    echo Uso: get_version.bat caminho\para\cobgdb.c
    exit /b 1
)

rem --- Verifica se o arquivo existe ---
if not exist "%~1" (
    echo Arquivo não encontrado: %~1
    exit /b 1
)

rem --- Lê a linha com #define COBGDB_VERSION ---
for /f "tokens=3 delims= " %%a in ('type "%~1" ^| findstr /R /C:"#define COBGDB_VERSION"') do (
    set "ver=%%~a"
)

rem --- Remove aspas e exibe ---
if defined ver (
    set "ver=!ver:"=!"
    echo !ver!
) else (
    echo 0.0.0
)

endlocal
exit /b 0
