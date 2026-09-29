@echo off
setlocal

REM ============================================================
REM IntelNet Qt client build script
REM
REM Usage:
REM   build.bat [QtPath]
REM Example:
REM   build.bat C:\Qt\6.5.3\msvc2019_64
REM If no argument is given, QTDIR is used, then the default
REM C:\Qt\6.5.3\msvc2019_64.
REM ============================================================

set "QT_DIR=%~1"
if "%QT_DIR%"=="" set "QT_DIR=%QTDIR%"
if "%QT_DIR%"=="" set "QT_DIR=C:\Qt\6.5.3\msvc2019_64"

if not exist "%QT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo [ERROR] Qt6 not found under "%QT_DIR%".
    echo         Pass the MSVC Qt path, e.g. build.bat C:\Qt\6.5.3\msvc2019_64
    exit /b 1
)

echo ============================================================
echo Using Qt: %QT_DIR%
echo ============================================================

cd /d "%~dp0"

if not exist build mkdir build
cd build

echo.
echo [1/3] Configuring CMake...
cmake .. -A x64 -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 (
    echo [ERROR] CMake configure failed.
    exit /b 1
)

echo.
echo [2/3] Building (this also builds the Rust core)...
cmake --build . --config Release
if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo.
echo [3/3] Done.
echo Executable: build\Release\IntelNetBrowser.exe
echo.
exit /b 0
