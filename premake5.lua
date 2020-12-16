workspace "Syndra-Engine"
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
IncludeDir["GLFW"] = "Syndra/vendor/GLFW/include"
IncludeDir["Glad"] = "Syndra/vendor/Glad/include"

include "Syndra/vendor/GLFW"
include "Syndra/vendor/Glad"

project "Syndra"
	location "Syndra"
	kind "SharedLib"
	language "C++"

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

	pchheader "lpch.h"
	pchsource "Syndra/src/lpch.cpp"
	

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}"
	}

	links
	{
		"GLFW",
		"Glad",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++latest"
		staticruntime "On"
		systemversion "latest"

	defines
	{
		"SN_PLATFORM_WINDOWS",
		"SYNDRA_BUILD_DLL",
		"GLFW_INCLUDE_NONE"
	}

	postbuildcommands
	{
		("{COPY} %{cfg.buildtarget.relpath} ../bin/" .. outputDir .. "/Sandbox")
	}

	filter "configurations:Debug"
		defines "SN_DEBUG"
		symbols "On"
		
	filter "configurations:release"
		defines "SN_RELEASE"
		optimize "On"	
		
	filter "configurations:Dist"
		defines "SN_DIST"
		optimize "On"


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
		"Syndra/vendor/spdlog/include",
		"Syndra/src"
	}

	links
	{
		"Syndra"
	}

	filter "system:windows"
		cppdialect "C++17"
		staticruntime "On"
		systemversion "latest"

	defines
	{
		"SN_PLATFORM_WINDOWS"
	}

	filter "configurations:Debug"
		defines "SN_DEBUG"
		symbols "On"
		
	filter "configurations:release"
		defines "SN_RELEASE"
		optimize "On"	
		
	filter "configurations:Dist"
		defines "SN_DIST"
		optimize "On"