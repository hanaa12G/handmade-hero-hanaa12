#!/bin/bash

mkdir -p build
pushd build

DFLAGS="-DHANDMADE_SLOW -DHANDMADE_INTERNAL"
WOPTIONS="-Wall -Wextra"
DEBUG_OPTIONS="-g -O0"


g++ \
    ${DFLAGS} \
    ${WOPTIONS} \
    ${DEBUG_OPTIONS} \
    -I../source \
    ../source/handmade.cpp \
    -fpic -shared \
    -o libhandmade.so

if [ $? -ne 0 ]; then
    echo "ERROR: Compile" >&2
    exit 1
fi

g++ \
    ${DFLAGS} \
    ${WOPTIONS} \
    ${DEBUG_OPTIONS} \
    -I../source \
    ../source/sdl_handmade.cpp \
    -lSDL3 \
    -Wl,-rpath /usr/local/lib \
    -o sdl_handmade

if [ $? -ne 0 ]; then
    echo "ERROR: Compile" >&2
    exit 1
fi


echo "COMPILE SUCCESS"

popd
