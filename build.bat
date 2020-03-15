@echo OFF
setlocal EnableDelayedExpansion 
set srcPath=%~dp0src\
set objPath=%~dp0obj\
set binPath=%~dp0bin\
set CompilerFlags=/D_USRDLL /D_WINDLL /nologo /Fo%objPath% /D_CRT_SECURE_NO_WARNINGS /Zi /std:c++17 /EHsc
set CompilerFlags=%CompilerFlags% /I "E:\Projects\DllInjectionLib\src"
set LinkerFlags=/DLL /SUBSYSTEM:CONSOLE /incremental:no /NOLOGO /OUT:%binPath%dll.dll
rem SymbolTable.cpp
set TUnits=dllmain.cpp dll.cpp "E:\Projects\dllInjectionLib\src\dllInj.cpp" fixer.cpp 

if not exist %objPath% mkdir %objPath%
if not exist %binPath% mkdir %binPath%

cd %srcPath%

set CompilerFlags=%CompilerFlags% /link %LinkerFlags%
cl %TUnits% %CompilerFlags%

del vc140.pdb

cd ..