@echo off
if not exist build mkdir build

set vcpkg=D:\vcpkg\installed\x64-windows

cl.exe /nologo /std:c11 /W4 /DDEBUG /I"%vcpkg%\include" /I./lib src\game.c /Febuild\game.exe /Fobuild\ ^
    /link /LIBPATH:"%vcpkg%\lib" glfw3dll.lib glew32.lib opengl32.lib gdi32.lib user32.lib shell32.lib

copy /y "%vcpkg%\bin\glew32.dll" build\ >nul
copy /y "%vcpkg%\bin\glfw3.dll" build\ >nul

echo Build complete!