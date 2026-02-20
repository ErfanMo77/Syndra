#include "lpch.h"
#include <Engine.h>
#include "Engine/Core/EntryPoint.h"
#include "EditorLayer.h"
#include <array>
#include <filesystem>
#include <vector>

namespace {

	bool IsUsableEditorRoot(const std::filesystem::path& root)
	{
		return std::filesystem::exists(root / "assets/fonts")
			&& std::filesystem::exists(root / "assets/shaders")
			&& std::filesystem::exists(root / "assets/shaders/vulkan");
	}

	void EnsureRuntimeAssetDirectories(const std::filesystem::path& root)
	{
		namespace fs = std::filesystem;
		const std::array<fs::path, 4> requiredDirectories = {
			root / "assets/Scenes",
			root / "assets/cache/shader",
			root / "assets/cache/shader/opengl",
			root / "assets/cache/shader/vulkan"
		};

		for (const fs::path& directory : requiredDirectories)
		{
			if (fs::exists(directory))
				continue;

			std::error_code errorCode;
			if (!fs::create_directories(directory, errorCode))
			{
				SN_WARN("Failed to create asset directory '{}': {}", directory.string(), errorCode.message());
				continue;
			}

			SN_INFO("Created missing asset directory '{}'.", directory.string());
		}
	}

	void ConfigureEditorWorkingDirectory()
	{
		namespace fs = std::filesystem;
		const fs::path cwd = fs::current_path();
		if (IsUsableEditorRoot(cwd))
		{
			EnsureRuntimeAssetDirectories(cwd);
			return;
		}

		std::vector<fs::path> candidates;
		candidates.reserve(16);
		candidates.push_back(cwd / "Syndra-Editor");

		fs::path cursor = cwd;
		while (!cursor.empty())
		{
			candidates.push_back(cursor);
			candidates.push_back(cursor / "Syndra-Editor");

			const fs::path parent = cursor.parent_path();
			if (parent == cursor)
				break;
			cursor = parent;
		}

		for (const fs::path& candidate : candidates)
		{
			if (IsUsableEditorRoot(candidate))
			{
				fs::current_path(candidate);
				SN_INFO("Working directory changed to '{}'.", fs::current_path().string());
				EnsureRuntimeAssetDirectories(fs::current_path());
				return;
			}
		}

		SN_WARN("Could not find an editor assets directory from '{}'.", cwd.string());
	}

}

namespace Syndra {

	class SyndraEditorApp : public Application
	{
	public:
		SyndraEditorApp()
			:Application("Syndra Editor")
		{
			PushLayer(new EditorLayer());
		}

		~SyndraEditorApp() {

		}

	};

	Application* CreateApplication() {
		ConfigureEditorWorkingDirectory();
		return new SyndraEditorApp();
	}

}
