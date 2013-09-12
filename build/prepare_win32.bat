@echo off
cmd /C "cd ..\external\STLPort-5.2.1 && configure msvc9 --clean"
cmd /C "cd ..\external\STLPort-5.2.1 && configure msvc9 --with-dynamic-rtl"
cmd /C "cd ..\external\STLPort-5.2.1\build\lib && nmake clean install"

