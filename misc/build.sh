#!/bin/bash

mkdir -p build
pushd build

x86_64-w64-mingw32-g++ -DHANDMADE_SLOW -DHANDMADE_INTERNAL -Wall -Wextra -I../source ../source/win32_handmade.cpp -mwindows -lwinmm -o win32_handmade.exe  -static-libgcc -static-libstdc++


cp win32_handmade.exe

popd
