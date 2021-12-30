#pragma once
#include "../UI.h"

#include <filesystem>

namespace Syndra {

	class ContentBrowser
	{

	public:
		ContentBrowser();

		void OnImGuiRender();


	private:
		Ref<Texture2D> m_SceneTexture;
		std::filesystem::path m_Directory;

		std::filesystem::directory_iterator m_DirIterator;
	};


}