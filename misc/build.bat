@echo off

mkdir \handmade-hero\build
pushd \handmade-hero\build

cl /Zi /FC ..\source\win32_handmade.cpp user32.lib gdi32.lib

popd
