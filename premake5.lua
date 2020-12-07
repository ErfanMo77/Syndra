workspace "Engine-test"
	architecture "x64"
	startproject "Sandbox" 
	configurations
	{
		"Debug",
		"release",
		"Dist"
	}



outputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] = "Engine-test/vendor/GLFW/include"



project "Engine-test"
	location "Engine-test"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

	pchheader "lpch.h"
	pchsource "Engine-test/src/lpch.cpp"
	

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}"
	}

	links
	{
		"GLFW",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

	defines
	{
		"FB_PLATFORM_WINDOWS",
		"ENGINE_BUILD_DLL"
	}

	postbuildcommands
	{
		("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputDir .. "/Sandbox")
	}

	filter "configurations:Debug"
		defines "FB_DEBUG"
		symbols "On"
		
	filter "configurations:release"
		defines "FB_RELEASE"
		optimize "On"	
		
	filter "configurations:Dist"
		defines "FB_DIST"
		optimize "On"

include "Engine-test/vendor/GLFW"

project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs 
	{
		"Engine-test/vendor/spdlog/include",
		"Engine-test/src"
	}

	links
	{
		"Engine-test"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

	defines
	{
		"FB_PLATFORM_WINDOWS"
	}

	filter "configurations:Debug"
		defines "FB_DEBUG"
		symbols "On"
		
	filter "configurations:release"
		defines "FB_RELEASE"
		optimize "On"	
		
	filter "configurations:Dist"
		defines "FB_DIST"
		optimize "On"