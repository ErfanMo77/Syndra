#include "lpch.h"
#include "ContentBrowser.h"
#include "Engine/ImGui/ImGuiLayer.h"

namespace Syndra {


	ContentBrowser::ContentBrowser()
		:m_Directory("assets")
	{
		m_SceneTexture = Texture2D::Create("assets/Icons/globe.png");
		m_DirIterator = std::filesystem::directory_iterator(m_Directory);
	}

	void ContentBrowser::OnImGuiRender()
	{
		ImGui::Begin(ICON_FA_BOOK" Content Browser");

		if (m_Directory != std::filesystem::path("assets"))
		{
			if (ImGui::Button("<-"))
			{
				m_Directory = m_Directory.parent_path();
			}
		}

		for (auto& directoryEntry : std::filesystem::directory_iterator(m_Directory))
		{
			const auto& path = directoryEntry.path();
			auto relativePath = std::filesystem::relative(path, "assets");
			std::string filenameString = relativePath.filename().string();
			ImGui::Text(filenameString.c_str());
			if (directoryEntry.is_directory())
			{
				if (ImGui::Button(filenameString.c_str()))
				{
					m_Directory /= path.filename();
					//m_DirIterator = std::filesystem::directory_iterator(m_Directory);
				}
			}
			else
			{
				ImGui::PushID(filenameString.c_str());
				if (ImGui::ImageButton("##FileButton", ImGuiLayer::GetTextureID(m_SceneTexture->GetRendererID()), { 40,40 }))
				{

				}
				ImGui::PopID();
			}

		}

		ImGui::End();
	}

}
