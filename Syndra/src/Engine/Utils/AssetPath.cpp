#include "lpch.h"

#include "Engine/Utils/AssetPath.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <unordered_map>
#include <vector>

namespace {

	std::string ToForwardSlashes(std::string value)
	{
		std::replace(value.begin(), value.end(), '\\', '/');
		return value;
	}

	bool EqualsNoCase(const std::string& a, const std::string& b)
	{
		if (a.size() != b.size())
			return false;

		for (size_t i = 0; i < a.size(); ++i)
		{
			if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
				return false;
		}

		return true;
	}

	std::vector<std::filesystem::path> GetEditorRoots()
	{
		std::vector<std::filesystem::path> roots;
		roots.reserve(4);

		const std::filesystem::path cwd = std::filesystem::current_path();
		roots.push_back(cwd);
		roots.push_back(cwd / "Syndra-Editor");
		if (cwd.has_parent_path())
		{
			roots.push_back(cwd.parent_path() / "Syndra-Editor");
			if (cwd.parent_path().has_parent_path())
				roots.push_back(cwd.parent_path().parent_path() / "Syndra-Editor");
		}

		std::vector<std::filesystem::path> uniqueRoots;
		uniqueRoots.reserve(roots.size());
		for (const auto& root : roots)
		{
			const auto normalized = root.lexically_normal();
			if (std::filesystem::exists(normalized) &&
				std::find(uniqueRoots.begin(), uniqueRoots.end(), normalized) == uniqueRoots.end())
			{
				uniqueRoots.push_back(normalized);
			}
		}

		return uniqueRoots;
	}

	bool TryResolvePath(const std::filesystem::path& candidate, std::string& resolvedPath)
	{
		if (!candidate.empty() && std::filesystem::exists(candidate))
		{
			resolvedPath = candidate.lexically_normal().string();
			return true;
		}

		return false;
	}

}

namespace Syndra {

	std::string AssetPath::ResolveEditorAssetPath(const std::string& inputPath)
	{
		if (inputPath.empty())
			return inputPath;

		std::string resolved = inputPath;
		const std::filesystem::path requestedPath(inputPath);
		if (TryResolvePath(requestedPath, resolved))
			return resolved;

		if (!requestedPath.is_relative())
			return inputPath;

		const auto roots = GetEditorRoots();
		for (const auto& root : roots)
		{
			if (TryResolvePath(root / requestedPath, resolved))
				return resolved;
		}

		return inputPath;
	}

	std::string AssetPath::ResolveTexturePath(const std::string& inputPath)
	{
		if (inputPath.empty())
			return inputPath;

		static std::unordered_map<std::string, std::string> s_PathCache;
		const auto cacheIt = s_PathCache.find(inputPath);
		if (cacheIt != s_PathCache.end())
			return cacheIt->second;

		std::string resolved = inputPath;
		const std::filesystem::path requestedPath(inputPath);
		const bool requestedPathExists = std::filesystem::exists(requestedPath);
		auto tryResolve = [&](const std::filesystem::path& candidate) -> bool
		{
			return TryResolvePath(candidate, resolved);
		};

		if (!tryResolve(requestedPath))
		{
			const auto roots = GetEditorRoots();
			if (requestedPath.is_relative())
			{
				for (const auto& root : roots)
				{
					if (tryResolve(root / requestedPath))
						break;
				}
			}

			if (resolved == inputPath)
			{
				const std::string normalizedInput = ToForwardSlashes(inputPath);
				constexpr const char* editorAssetsMarker = "Syndra-Editor/assets/";
				constexpr const char* assetsMarker = "assets/";

				const size_t editorAssetsPos = normalizedInput.find(editorAssetsMarker);
				if (editorAssetsPos != std::string::npos)
				{
					const std::string suffix = normalizedInput.substr(editorAssetsPos + std::strlen(editorAssetsMarker));
					for (const auto& root : roots)
					{
						if (tryResolve(root / "assets" / suffix))
							break;
					}
				}

				if (resolved == inputPath)
				{
					const size_t assetsPos = normalizedInput.find(assetsMarker);
					if (assetsPos != std::string::npos)
					{
						const std::string suffix = normalizedInput.substr(assetsPos + std::strlen(assetsMarker));
						for (const auto& root : roots)
						{
							if (tryResolve(root / "assets" / suffix))
								break;
						}
					}
				}
			}

			if (resolved == inputPath)
			{
				const std::string filename = requestedPath.filename().string();
				if (!filename.empty())
				{
					for (const auto& root : GetEditorRoots())
					{
						const std::filesystem::path assetsRoot = root / "assets";
						if (!std::filesystem::exists(assetsRoot))
							continue;

						bool found = false;
						for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsRoot))
						{
							if (!entry.is_regular_file())
								continue;

							if (EqualsNoCase(entry.path().filename().string(), filename))
							{
								resolved = entry.path().lexically_normal().string();
								found = true;
								break;
							}
						}

						if (found)
							break;
					}
				}
			}
		}

		if (!requestedPathExists && ToForwardSlashes(resolved) != ToForwardSlashes(inputPath))
		{
			SN_CORE_WARN("Resolved texture path '{}' -> '{}'.", inputPath, resolved);
		}

		s_PathCache[inputPath] = resolved;
		return resolved;
	}

}
