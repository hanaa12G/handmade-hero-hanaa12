@echo off

mkdir \handmade-hero\build
pushd \handmade-hero\build

cl /DHANDMADE_SLOW /DHANDMADE_INTERNAL /Zi /FC /I..\source\  ..\source\win32_handmade.cpp  user32.lib gdi32.lib

popd
