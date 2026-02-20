#pragma once

#include <string>

namespace Syndra {

	class AssetPath
	{
	public:
		static std::string ResolveEditorAssetPath(const std::string& inputPath);
		static std::string ResolveTexturePath(const std::string& inputPath);
	};

}
