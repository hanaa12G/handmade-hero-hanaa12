#!/bin/bash

mkdir -p build
pushd build

x86_64-w64-mingw32-g++ -DHANDMADE_SLOW -DHANDMADE_INTERNAL -Wall -Wextra -I../source ../source/win32_handmade.cpp -mwindows -lwinmm -o win32_handmade.exe  -static-libgcc -static-libstdc++

if [ $? -ne 0 ]; then
    echo "ERROR: Compile" >&2
    exit 1
fi

echo "COMPILE SUCCESS"

popd
