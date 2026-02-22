#include "lpch.h"
#include "MeshPanel.h"
#include "Engine/Utils/PlatformUtils.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/GltfSceneImporter.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace Syndra {

	MeshPanel::MeshPanel()
	{
	}

	void MeshPanel::DrawMesh(Entity& entity)
	{
		static bool MeshRemoved = false;
		if (UI::DrawComponent<MeshComponent>(ICON_FA_CUBE" Mesh", entity, true, &MeshRemoved)) {
			ImGui::Separator();
			auto& tag = entity.GetComponent<MeshComponent>().path;

			char buffer[256];
			memset(buffer, 0, sizeof(buffer));
			strcpy_s(buffer, tag.c_str());
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 2,5 });
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 5,0 });
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - 80);
			if (ImGui::InputText("##Path", buffer, sizeof(buffer))) {
				tag = std::string(buffer);
			}
			ImGui::PopStyleVar(2);
			ImGui::SameLine();
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 5,4 });
			if (ImGui::Button("Open")) {
				auto path = FileDialogs::OpenFile("Syndra Model (*.*)\0*.*\0");
				if (path) {
					auto dir = std::filesystem::current_path();
					std::string filePath;
					if (path->find(dir.string()) != std::string::npos) {
						filePath = path->substr(dir.string().size());
					}
					else {
						filePath = *path;
					}

					auto& meshComponent = entity.GetComponent<MeshComponent>();
					Scene* scene = Entity::s_Scene;
					const std::string extension = std::filesystem::path(*path).extension().string();
					std::string lowerExtension = extension;
					std::transform(lowerExtension.begin(), lowerExtension.end(), lowerExtension.begin(), [](unsigned char c) {
						return static_cast<char>(std::tolower(c));
					});
					if (scene && (lowerExtension == ".gltf" || lowerExtension == ".glb"))
					{
						if (!GltfSceneImporter::ImportIntoEntity(*scene, entity, *path))
						{
							SN_CORE_WARN("Failed to import glTF model '{}'.", *path);
						}
					}
					else
					{
						Model loadedModel(*path);
						tag = filePath;
						meshComponent.MeshIndex = -1;
						meshComponent.MeshName.clear();
						meshComponent.model = std::move(loadedModel);
					}
				}
			}
			ImGui::PopStyleVar();
			ImGui::TreePop();
		}
		if (MeshRemoved) {
			entity.RemoveComponent<MeshComponent>();
			MeshRemoved = false;
		}
	}

}

