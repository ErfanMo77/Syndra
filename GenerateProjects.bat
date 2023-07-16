call xcopy /Q /E /Y /I Syndra\vendor\assimp\build\Debug\assimp-vc142-mtd.dll bin\Debug-windows-x86_64\Syndra-Editor\
call xcopy /Q /E /Y /I %VULKAN_SDK%\Bin bin\Debug-windows-x86_64\Syndra-Editor\
call xcopy /Q /E /Y /I Syndra\vendor\assimp\build\Release\assimp-vc142-mt.dll bin\release-windows-x86_64\Syndra-Editor\
call xcopy /Q /E /Y /I %VULKAN_SDK%\Bin bin\release-windows-x86_64\Syndra-Editor\
call vendor\bin\premake\premake5.exe vs2022
PAUSE