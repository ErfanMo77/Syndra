workspace "Engine-test"
	architecture "x64"

	configurations
	{
		"Debug",
		"release",
		"Dist"
	}

startproject "Sandbox" 
outputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Engine-test"
	location "Engine-test"
	kind "SharedLib"
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
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include"
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