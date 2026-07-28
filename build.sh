#!/bin/bash

set -e

if [ "$(uname)" == "Linux" ]; then
    LIBS=(-lglfw -lGLEW -lGL -lX11 -lpthread -ldl -lm)
    CC="gcc"
elif [ "$(uname)" == "Darwin" ]; then
    LIBS=(-lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo)
    CC="clang"
else
    echo "UNSUPPORTED OS"
    exit 1
fi

mkdir -p bin

MODE="$1"

case "$MODE" in
    debug)
        FLAGS=(-g -O0 -DDEBUG -Wall -Wextra -std=c99)
        ;;
    dev)
        FLAGS=(-g -O0 -Wall -Wextra -std=c99)
        ;;
    release)
        FLAGS=(-O3 -Wall -Wextra -std=c99)
        ;;
    *)
        echo "BUILD AVAILABLE OPTIONS: debug, dev, release"
        exit 1
        ;;
esac

rm -rf build
mkdir -p build

$CC "${FLAGS[@]}" \
    -I./lib \
    src/game.c \
    "${LIBS[@]}" \
    -o build/arena

rm -f bin/arena
rm -rf bin/res

if [ "$MODE" = "release" ]; then
    cp -f build/arena bin/
    if [ -d "res" ]; then
        if command -v rsync >/dev/null 2>&1; then
            rsync -av --delete res/ bin/res/ >/dev/null
        else
            cp -R res bin/res
        fi
    fi

    if [ "$(uname)" == "Linux" ]; then
        mkdir -p $HOME/.local/share/applications
        mkdir -p $HOME/.local/share/icons
        cp -f bin/res/arena.png $HOME/.local/share/icons/arena.png

        cat > $HOME/.local/share/applications/arena.desktop << EOF
[Desktop Entry]
Type=Application
Name=Arena
Exec=$(pwd)/bin/arena
Icon=$HOME/.local/share/icons/arena.png
Terminal=false
Categories=Game;
StartupWMClass=arena
EOF

        update-desktop-database $HOME/.local/share/applications
    fi
else
    ln -sf ../build/arena bin/arena
    if [ -d "res" ]; then
        ln -s ../res bin/res
    fi
fi

echo "BUILD COMPLETE"