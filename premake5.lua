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

VULKAN_SDK = os.getenv("VULKAN_SDK")

IncludeDir = {}
IncludeDir["GLFW"] =  "%{wks.location}/Syndra/vendor/GLFW/include"
IncludeDir["Glad"] =  "%{wks.location}/Syndra/vendor/Glad/include"
IncludeDir["imgui"] = "%{wks.location}/Syndra/vendor/imgui"
IncludeDir["glm"] = "%{wks.location}/Syndra/vendor/glm"
IncludeDir["stb_image"] = "%{wks.location}/Syndra/vendor/stb_image"
IncludeDir["entt"] = "%{wks.location}/Syndra/vendor/entt/Include"
IncludeDir["yaml_cpp"] = "%{wks.location}/Syndra/vendor/yaml-cpp/include"
IncludeDir["ImGuizmo"] = "%{wks.location}/Syndra/vendor/ImGuizmo"
IncludeDir["assimp"] = "%{wks.location}/Syndra/vendor/assimp/include"
IncludeDir["VulkanSDK"] = "%{VULKAN_SDK}/Include"
IncludeDir["shaderc"] = "%{wks.location}/Syndra/vendor/shaderc/include"
IncludeDir["SPIRV_Cross"] = "%{wks.location}/Syndra/vendor/SPIRV-Cross"


LibraryDir = {}

LibraryDir["VulkanSDK"] = "%{VULKAN_SDK}/Lib"

Library = {}
Library["Vulkan"] = "%{LibraryDir.VulkanSDK}/vulkan-1.lib"
Library["VulkanUtils"] = "%{LibraryDir.VulkanSDK}/VkLayer_utils.lib"

Library["ShaderC_Debug"] = "%{LibraryDir.VulkanSDK}/shaderc_sharedd.lib"
Library["SPIRV_Cross_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-cored.lib"
Library["SPIRV_Cross_GLSL_Debug"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsld.lib"
Library["SPIRV_Tools_Debug"] = "%{LibraryDir.VulkanSDK}/SPIRV-Toolsd.lib"

Library["ShaderC_Release"] = "%{LibraryDir.VulkanSDK}/shaderc_shared.lib"
Library["SPIRV_Cross_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-core.lib"
Library["SPIRV_Cross_GLSL_Release"] = "%{LibraryDir.VulkanSDK}/spirv-cross-glsl.lib"

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
	staticruntime "off"

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
		"%{IncludeDir.ImGuizmo}",
		"%{IncludeDir.assimp}",
		"%{IncludeDir.VulkanSDK}"
	}

	libdirs {
		"Syndra/vendor/assimp/build/Debug",
		"Syndra/vendor/assimp/build/Release"
	}

	links
	{
		"GLFW",
		"Glad",
		"imgui",
		"yaml-cpp",
		"opengl32.lib",
		"%{Library.Vulkan}"
	}
	
	filter "files:Syndra/vendor/ImGuizmo/**.cpp"
	flags { "NoPCH" }

	filter "system:windows"
		cppdialect "C++latest"
		systemversion "latest"

	defines
	{
		"GLFW_INCLUDE_NONE"
	}

	filter "configurations:Debug"
		defines "SN_DEBUG"
		symbols "on"
		links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}",
			"assimp-vc142-mtd.lib"
		}
		
	filter "configurations:release"
		defines "SN_RELEASE"
		optimize "on"
		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
			"assimp-vc142-mt.lib"
		}		
		
	filter "configurations:Dist"
		defines "SN_DIST"
		optimize "on"
		links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
			"assimp-vc142-mt.lib"
		}


project "Sandbox"
	location "Sandbox"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "off"

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
		"%{IncludeDir.imgui}",
		"%{IncludeDir.entt}",
		"%{IncludeDir.assimp}"
	}

	links
	{
		"Syndra"
	}

	filter "system:windows"
		staticruntime "off"
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
