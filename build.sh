#!/bin/bash

set -e

mkdir -p build

if [ "$(name)" == "Darwin" ]; then
    clang src/game.c -o build/game -std=c99 -Wall -Wextra \
        -I./lib -lglfw -framework Cocoa -framework OpenGL -framework IOKit
else
    gcc src/game.c -o build/game -std=c99 -Wall -Wextra \
        -I./lib -I/usr/include/GLFW -I/usr/include/GL \
        -lglfw -lGLEW -lGL -lm
fi

echo "Build complete!"