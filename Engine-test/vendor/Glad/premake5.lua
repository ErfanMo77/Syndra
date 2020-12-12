project "Glad"
	kind "StaticLib"
	language "C"
	outdir = "Debug-windows-x86_64"
	targetdir ("C:/Users/MSI/source/repos/Engine-test/bin/" .. outdir .. "/%{prj.name}")
	objdir ("C:/Users/MSI/source/repos/Engine-test/bin-int/" .. outdir .. "/%{prj.name}")

	files
	{
		"include/glad/glad.h",
        "include/KHR/khrplatform.h",
        "src/glad.c"
	}
	
    includedirs
    {
        "include"
    }
    
    filter "system:windows"
        systemversion "latest"

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"