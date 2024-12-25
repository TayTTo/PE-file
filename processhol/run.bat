@echo off

cl /EHsc /W4 /Od /Zi /D "_DEBUG" /D "_WINDOWS" /D "TARGETVER=0x0A00" /MTd /wd4005 /std:c++17 /D_AMD64_ /D_WIN64 .\processhollow.cpp /Feprocesshollowing.exe
