@echo off
setlocal

REM 设置 Visual Studio 环境
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat"

REM 编译测试程序
cl /nologo /EHsc test_ffi.cpp rust-core\target\release\intelnet_core.dll.lib ws2_32.lib userenv.lib bcrypt.lib ntdll.lib

if %ERRORLEVEL% equ 0 (
    echo.
    echo === Compilation SUCCESS ===
    echo.

    REM 复制 DLL
    copy rust-core\target\release\intelnet_core.dll . >nul

    echo Running test...
    echo.
    test_ffi.exe
) else (
    echo.
    echo === Compilation FAILED ===
)

endlocal
