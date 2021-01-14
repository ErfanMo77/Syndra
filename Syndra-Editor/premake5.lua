project "Syndra-Editor"
	kind "ConsoleApp"
	language "C++"
	cppdialect "C++17"
	staticruntime "on"
	
	targetdir ("%{wks.location}/bin/" .. outputDir .. "/%{prj.name}")
	objdir ("%{wks.location}/bin-int/" .. outputDir .. "/%{prj.name}")
	
	files
	{
		"src/**.h",
		"src/**.cpp"
	}

	includedirs
	{
		"%{wks.location}/Syndra/vendor/spdlog/include",
		"%{wks.location}/Syndra/src",
		"%{wks.location}/Syndra/vendor",
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
		runtime "Debug"
		symbols "on"

	filter "configurations:Release"
		defines "SN_RELEASE"
		runtime "Release"
		optimize "on"

	filter "configurations:Dist"
		defines "SN_DIST"
		runtime "Release"
		optimize "on"