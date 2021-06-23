call xcopy /Q /E /Y /I Syndra\vendor\assimp\build\Debug\assimp-vc142-mtd.dll bin\Debug-windows-x86_64\Syndra-Editor\
call xcopy /Q /E /Y /I Syndra\vendor\VulkanSDK\Bin\Debug bin\Debug-windows-x86_64\Syndra-Editor\
call xcopy /Q /E /Y /I Syndra\vendor\assimp\build\Release\assimp-vc142-mt.dll bin\release-windows-x86_64\Syndra-Editor\
call xcopy /Q /E /Y /I Syndra\vendor\VulkanSDK\Bin\Release bin\release-windows-x86_64\Syndra-Editor\
call vendor\bin\premake\premake5.exe vs2019
PAUSE