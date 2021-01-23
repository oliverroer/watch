@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
set output=bin
set compilerflags=/Od /Zi /EHsc /Fd%output%/ /Fo%output%/
mkdir %output%
set linkerflags=/OUT:%output%/clipboard.exe 
cl.exe %compilerflags% src/clipboard.c /link User32.lib %linkerflags%