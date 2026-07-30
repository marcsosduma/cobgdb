@echo off
setlocal enabledelayedexpansion

REM =========================
REM AUTOMATIC CONFIGURATION
REM =========================
set DIST=dist
set EXE=wcobgdb.exe

echo [0/5] Searching for MSYS2 installation...
set "MSYS_ROOT="

:: Scan the most common drives looking for the msys64 folder
for %%d in (C D E F G) do (
    if exist "%%d:\msys64\ucrt64\bin" (
        set "MSYS_ROOT=%%d:\msys64"
        goto :msys_found
    )
)

:msys_found
if "%MSYS_ROOT%"=="" (
    echo   [ERROR]: MSYS2 was not found on drives C, D, E, F, or G.
    echo   Please install MSYS2 or configure the path manually in this script.
    pause
    exit /b 1
)

set MSYS2=%MSYS_ROOT%\ucrt64
echo   - MSYS2 (UCRT64) found at: %MSYS2%
echo.

REM =========================
REM DISTRIBUTION START
REM =========================

echo [1/5] Cleaning target folder...
if exist "%DIST%" rmdir /s /q "%DIST%"
mkdir "%DIST%"
mkdir "%DIST%\lib"
mkdir "%DIST%\share"

echo [2/5] Copying Executable and DLLs to root...
copy "%EXE%" "%DIST%\" >nul

for %%f in (
    libgtk-3-0.dll libgdk-3-0.dll libgdk_pixbuf-2.0-0.dll libglib-2.0-0.dll 
    libgio-2.0-0.dll libgobject-2.0-0.dll libgmodule-2.0-0.dll libintl-8.dll 
    libcairo-2.dll libpango-1.0-0.dll libpangocairo-1.0-0.dll libpangowin32-1.0-0.dll 
    libharfbuzz-0.dll libfontconfig-1.dll libfreetype-6.dll libepoxy-0.dll 
    libpng16-16.dll zlib1.dll libffi-8.dll libiconv-2.dll libatk-1.0-0.dll 
    librsvg-2-2.dll libxml2-16.dll libfribidi-0.dll libpixman-1-0.dll 
    libjpeg-8.dll libtiff-6.dll libwebp-7.dll libgcc_s_seh-1.dll libstdc++-6.dll 
    libwinpthread-1.dll libpcre2-8-0.dll libcairo-gobject-2.dll libthai-0.dll 
    libpangoft2-1.0-0.dll libdatrie-1.dll libexpat-1.dll libbrotlidec.dll 
    libbrotlicommon.dll libbz2-1.dll libgraphite2.dll libdeflate.dll 
    libjbig-0.dll libLerc.dll liblzma-5.dll libzstd.dll libsharpyuv-0.dll
) do (
    if exist "%MSYS2%\bin\%%f" (
        copy "%MSYS2%\bin\%%f" "%DIST%\" >nul
    )
)

echo [3/5] Copying Support Folders...
xcopy /E /I /Y "%MSYS2%\lib\gdk-pixbuf-2.0" "%DIST%\lib\gdk-pixbuf-2.0" >nul
xcopy /E /I /Y "%MSYS2%\share\glib-2.0" "%DIST%\share\glib-2.0" >nul
REM xcopy /E /I /Y "%MSYS2%\share\icons" "%DIST%\share\icons" >nul
copy "samples\*" "%DIST%\" >nul

REM if exist "icons" (
REM     mkdir "%DIST%\icons"
REM     xcopy /E /I /Y "icons" "%DIST%\icons" >nul
REM )

if exist "samples" (
    mkdir "%DIST%\samples"
    xcopy /E /I /Y "samples" "%DIST%\samples" >nul
)

echo [4/5] Finalizing configurations...
"%MSYS2%\bin\glib-compile-schemas.exe" "%DIST%\share\glib-2.0\schemas"
"%MSYS2%\bin\gdk-pixbuf-query-loaders.exe" > "%DIST%\lib\gdk-pixbuf-2.0\2.10.0\loaders.cache"

echo [5/5] Searching for Inno Setup installation and compiling...

:: 1. Try using the 'where' iscc 
where iscc.exe >nul 2>nul
if !errorlevel! equ 0 (
    for /f "delims=" %%i in ('where iscc.exe') do set "INNO_PATH=%%i"
    goto :executar_inno
)

:: 2. Not found -> check default
if exist "C:\Program Files (x86)\Inno Setup 6\iscc.exe" (
    set "INNO_PATH=C:\Program Files (x86)\Inno Setup 6\iscc.exe"
    goto :executar_inno
)
if exist "C:\Program Files (x86)\Inno Setup 5\iscc.exe" (
    set "INNO_PATH=C:\Program Files (x86)\Inno Setup 5\iscc.exe"
    goto :executar_inno
)

echo   [ERROR]: Inno Setup (iscc.exe) was not found in PATH or default locations.
goto :fim_erro

:executar_inno
echo   - Inno Setup located at: "%INNO_PATH%"

:: 3. Verify if the Inno Setup script exists in the correct folder (GTK)
if not exist ".\setup_win.iss" (
    echo   [ERROR]: The file "setup_win.iss" was not found.
    echo   Verify if the folder GTK\setup_win.iss exists at: %cd%
    goto :fim_erro
)

:: 4. Run compilation pointing to the GTK directory
"%INNO_PATH%" ".\setup_win.iss"

if not errorlevel 1 (
    echo.
    echo =======================================================
    echo FINISHED SUCCESSFULLY!
    echo The executable and all DLLs are inside the 'dist' folder.
    echo The final installer (.exe) was generated successfully!
    echo =======================================================
    pause
    exit /b 0
)

:fim_erro
echo =======================================================
echo Automation finished with errors. The installer was not created.
echo =======================================================
pause
exit /b 1
