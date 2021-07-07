#include "lpch.h"
#include "ContentBrowser.h"

namespace Syndra {


	ContentBrowser::ContentBrowser()
		:m_Directory("assets/Scenes")
	{
		m_SceneTexture = Texture2D::Create("assets/Textures/scene.png");
	}

	void ContentBrowser::OnImGuiRender()
	{
		ImGui::Begin(ICON_FA_BOOK" Content Browser");

		if (m_Directory != std::filesystem::path("assets/Scenes"))
		{
			if (ImGui::Button("<-"))
			{
				m_Directory = m_Directory.parent_path();
			}
		}

		for (auto& directoryEntry : std::filesystem::directory_iterator(m_Directory))
		{

			const auto& path = directoryEntry.path();
			auto relativePath = std::filesystem::relative(path, "assets/Scenes");
			std::string filenameString = relativePath.filename().string();
			if (directoryEntry.is_directory())
			{
				if (ImGui::Button(filenameString.c_str()))
				{
					m_Directory /= path.filename();
				}
			}
			else
			{
				ImGui::Text(filenameString.c_str());
				if (ImGui::ImageButton(reinterpret_cast<void*>(m_SceneTexture->GetRendererID()), { 40,40 }))
				{

				}
				ImGui::SameLine();
			}

		}

		ImGui::End();
	}

}