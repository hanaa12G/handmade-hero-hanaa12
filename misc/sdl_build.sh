#!/bin/bash

mkdir -p build
pushd build

g++ \
    -DHANDMADE_SLOW -DHANDMADE_INTERNAL \
    -Wall -Wextra \
    -I../source \
    ../source/sdl_handmade.cpp \
    -g -O0 \
    -lSDL3 \
    -Wl,-rpath /usr/local/lib \
    -o sdl_handmade

if [ $? -ne 0 ]; then
    echo "ERROR: Compile" >&2
    exit 1
fi

echo "COMPILE SUCCESS"

popd
