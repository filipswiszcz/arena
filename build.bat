@echo off

set "vcpkg=D:\vcpkg\installed\x64-windows"
set "mode=%1"

if not exist bin mkdir bin

if "%mode%"=="debug" goto debug
if "%mode%"=="dev" goto dev
if "%mode%"=="release" goto release

echo BUILD AVAILABLE OPTIONS: debug, dev, release
goto eof

:debug
set "flags=/Zi /Od /MDd /DDEBUG /W4"
set "lflags=/SUBSYSTEM:CONSOLE"
goto compile

:dev
set "flags=/Zi /Od /MDd /W4"
set "lflags=/SUBSYSTEM:CONSOLE"
goto compile

:release
set "flags=/O2 /MD /W4"
set "lflags=/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
goto compile

:compile
if exist build rmdir /s /q build
mkdir build

cl.exe /nologo /std:c11 %flags% ^
    /I"%vcpkg%\include" ^
    /I.\lib ^
    src\game.c ^
    /Fobuild\ ^
    /Fdbuild\ ^
    /Febuild\game.exe ^
    /link ^
    %lflags% ^
    /LIBPATH:"%vcpkg%\lib" ^
    glfw3dll.lib glew32.lib opengl32.lib gdi32.lib user32.lib shell32.lib

if %errorlevel% neq 0 (
    echo BUILD ERROR
    exit /b %errorlevel%
)

copy /y "%vcpkg%\bin\glew32.dll" bin\ >nul
copy /y "%vcpkg%\bin\glfw3.dll" bin\ >nul

copy /y build\game.exe bin\ >nul

if exist bin\res rmdir /s /q bin\res

if "%mode%"=="release" (
    if exist res (
        robocopy res bin\res /mir >nul
    )
) else (
    if exist res (
        mklink /j bin\res res >nul
    )
)

echo BUILD COMPLETE

:eof