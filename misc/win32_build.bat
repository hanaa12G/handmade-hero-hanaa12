@echo off

mkdir \handmade-hero\build
pushd \handmade-hero\build

cl -nologo -W4 /Gm- /EHa- /DHANDMADE_SLOW /DHANDMADE_INTERNAL /MT /Oi /Zi /FC /wd4201 /I..\source\  ..\source\win32_handmade.cpp user32.lib gdi32.lib Winmm.lib

popd
