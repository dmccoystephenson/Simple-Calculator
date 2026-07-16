@echo off
REM ===========================================================================
REM  One-shot build-and-run for the Simple Calculator GUI on Windows.
REM
REM  Targets the MinGW-w64 toolchain (the same g++ + pkg-config build used by
REM  the Makefile). The easiest way to get it is MSYS2: https://www.msys2.org/
REM  In an MSYS2 MinGW64 shell, install the prerequisites once:
REM
REM    pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-SDL2 ^
REM             mingw-w64-x86_64-SDL2_image mingw-w64-x86_64-pkg-config
REM
REM  then add  C:\msys64\mingw64\bin  to your PATH so this script (run from a
REM  normal Command Prompt or by double-clicking) can find g++ and the SDL DLLs.
REM
REM  Double-click this file, or run  build-and-run.bat  from a terminal.
REM ===========================================================================

setlocal enabledelayedexpansion

REM Run from this script's own directory so the PNG assets resolve (IMG_Load
REM uses relative paths).
cd /d "%~dp0"

REM --- check for the compiler ---------------------------------------------
where g++ >nul 2>nul
if errorlevel 1 (
    echo [error] g++ was not found on your PATH.
    echo         Install MinGW-w64 ^(via MSYS2: https://www.msys2.org/^), then add
    echo         C:\msys64\mingw64\bin to your PATH. See this script's header for
    echo         the exact pacman command.
    exit /b 1
)

REM --- check for pkg-config (used to locate SDL2) -------------------------
where pkg-config >nul 2>nul
if errorlevel 1 (
    echo [error] pkg-config was not found on your PATH ^(needed to locate SDL2^).
    echo         Install it:  pacman -S mingw-w64-x86_64-pkg-config
    exit /b 1
)

REM --- resolve the SDL2 + SDL2_image build flags -------------------------
set "SDL_FLAGS="
for /f "delims=" %%F in ('pkg-config --cflags --libs sdl2 SDL2_image 2^>nul') do set "SDL_FLAGS=%%F"
if "!SDL_FLAGS!"=="" (
    echo [error] SDL2 / SDL2_image were not found by pkg-config.
    echo         Install them:
    echo           pacman -S mingw-w64-x86_64-SDL2 mingw-w64-x86_64-SDL2_image
    exit /b 1
)

REM --- build --------------------------------------------------------------
echo Building simpleCalculator.exe ...
g++ -std=c++17 simpleCalculator.cpp CalculatorEngine.cpp -o simpleCalculator.exe !SDL_FLAGS!
if errorlevel 1 (
    echo [error] Build failed ^(see the compiler output above^).
    exit /b 1
)
echo Build succeeded.

REM --- run ----------------------------------------------------------------
echo Starting Simple Calculator ...
simpleCalculator.exe

endlocal
