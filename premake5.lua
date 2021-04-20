workspace "Syndra-Engine"
	architecture "x64"
	startproject "Syndra-Editor" 
	configurations
	{
		"Debug",
		"release",
		"Dist"
	}

outputDir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

IncludeDir = {}
IncludeDir["GLFW"] =  "%{wks.location}/Syndra/vendor/GLFW/include"
IncludeDir["Glad"] =  "%{wks.location}/Syndra/vendor/Glad/include"
IncludeDir["imgui"] = "%{wks.location}/Syndra/vendor/imgui"
IncludeDir["glm"] = "%{wks.location}/Syndra/vendor/glm"
IncludeDir["stb_image"] = "%{wks.location}/Syndra/vendor/stb_image"
IncludeDir["entt"] = "%{wks.location}/Syndra/vendor/entt/Include"
IncludeDir["yaml_cpp"] = "%{wks.location}/Syndra/vendor/yaml-cpp/include"
IncludeDir["ImGuizmo"] = "%{wks.location}/Syndra/vendor/ImGuizmo"

group "Dependencies"
	include "Syndra/vendor/GLFW"
	include "Syndra/vendor/Glad"
	include "Syndra/vendor/imgui"
	include "Syndra/vendor/yaml-cpp"
group ""

include "Syndra-Editor"

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
		"%{prj.name}/vendor/stb_image/**.cpp",
		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.h",
		"%{prj.name}/vendor/ImGuizmo/ImGuizmo.cpp"
	}

	includedirs
	{
		"%{prj.name}/src",
		"%{prj.name}/vendor/spdlog/include",
		"%{IncludeDir.GLFW}",
		"%{IncludeDir.Glad}",
		"%{IncludeDir.imgui}",
		"%{IncludeDir.glm}",
		"%{IncludeDir.stb_image}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.yaml_cpp}",
		"%{IncludeDir.ImGuizmo}"
	}

	links
	{
		"GLFW",
		"Glad",
		"imgui",
		"yaml-cpp",
		"opengl32.lib",
	}
	
	filter "files:Syndra/vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }

	filter "system:windows"
		cppdialect "C++latest"
		staticruntime "on"
		systemversion "latest"

	defines
	{
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

	filter "configurations:Debug"
		defines "SN_DEBUG"
		symbols "on"
		
	filter "configurations:release"
		defines "SN_RELEASE"
		optimize "on"	
		
	filter "configurations:Dist"
		defines "SN_DIST"
		optimize "on"