@echo off
if not exist build mkdir build

cl.exe /nologo /std:c11 /W4 /I.\lib src\game.c /Febuild\game.exe /Fobuild\ ^
    glfw3dll.lib glew32.lib opengl32.lib gdi32.lib user32.lib shell32.lib

echo Build complete!