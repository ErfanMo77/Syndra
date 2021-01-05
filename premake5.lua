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
IncludeDir["GLFW"] =  "Syndra/vendor/GLFW/include"
IncludeDir["Glad"] =  "Syndra/vendor/Glad/include"
IncludeDir["imgui"] = "Syndra/vendor/imgui"
IncludeDir["glm"] =   "Syndra/vendor/glm"
IncludeDir["stb_image"] =   "Syndra/vendor/stb_image"

include "Syndra/vendor/GLFW"
include "Syndra/vendor/Glad"
include "Syndra/vendor/imgui"

project "Syndra"
	location "Syndra"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

	targetdir ("bin/" .. outputDir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputDir .. "/%{prj.name}")

	pchheader "lpch.h"
	pchsource "Syndra/src/lpch.cpp"
	

	files
	{
		"%{prj.name}/src/**.h",
		"%{prj.name}/src/**.cpp",
		"%{prj.name}/vendor/glm/glm/**.hpp",
		"%{prj.name}/vendor/glm/glm/**.inl",
		"%{prj.name}/vendor/stb_image/**.h",
		"%{prj.name}/vendor/stb_image/**.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.imgui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}"
	}

	links
	{
		"GLFW",
		"Glad",
		"imgui",
		"opengl32.lib"
	}

	filter "system:windows"
		cppdialect "C++latest"
		staticruntime "on"
		systemversion "latest"

	defines
	{
		"SN_PLATFORM_WINDOWS",
		"SYNDRA_BUILD_DLL",
		"GLFW_INCLUDE_NONE"
	}

	filter "configurations:Debug"
		defines "SN_DEBUG"
		symbols "on"
		
	filter "configurations:release"
		defines "SN_RELEASE"
		optimize "on"	
		
	filter "configurations:Dist"
		defines "SN_DIST"
		optimize "on"


project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"

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
		"Syndra/src",
		"%{IncludeDir.glm}",
		"%{IncludeDir.imgui}"
	}

	links
	{
		"Syndra"
	}

	filter "system:windows"
		staticruntime "on"
		systemversion "latest"

	defines
	{
		"SN_PLATFORM_WINDOWS"
	}

	filter "configurations:Debug"
		defines "SN_DEBUG"
		symbols "on"
		
	filter "configurations:release"
		defines "SN_RELEASE"
		optimize "on"	
		
	filter "configurations:Dist"
		defines "SN_DIST"
		optimize "on"