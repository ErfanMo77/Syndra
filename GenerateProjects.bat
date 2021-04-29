call xcopy /Q /E /Y /I Syndra\vendor\assimp\build\assimp-vc142-mtd.dll bin\Debug-windows-x86_64\Syndra-Editor\
call vendor\bin\premake\premake5.exe vs2019
PAUSE