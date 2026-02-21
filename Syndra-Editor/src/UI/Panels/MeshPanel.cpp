#include "lpch.h"
#include "MeshPanel.h"
#include "Engine/Utils/PlatformUtils.h"
#include "Engine/Scene/Entity.h"

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
				auto dir = std::filesystem::current_path();
				if (path) {
					std::string filePath;
					if (path->find(dir.string()) != std::string::npos) {
						filePath = path->substr(dir.string().size());
					}
					else {
						filePath = *path;
					}

					Model loadedModel(*path);
					auto& meshComponent = entity.GetComponent<MeshComponent>();
					Scene* scene = Entity::s_Scene;

					// Import multi-mesh models as a hierarchy of child entities instead of a single flattened mesh owner.
					if (scene && loadedModel.meshes.size() > 1)
					{
						std::string baseName = entity.GetComponent<TagComponent>().Tag;
						if (entity.HasComponent<RelationshipComponent>())
						{
							auto children = entity.GetComponent<RelationshipComponent>().Children;
							for (entt::entity child : children)
								scene->DestroyEntity(Entity{ child });
						}

						meshComponent.path.clear();
						meshComponent.model = Model{};
						if (entity.HasComponent<MaterialComponent>())
							entity.RemoveComponent<MaterialComponent>();

						for (size_t meshIndex = 0; meshIndex < loadedModel.meshes.size(); ++meshIndex)
						{
							auto child = scene->CreateEntity(baseName + "_Part" + std::to_string(meshIndex));
							auto& childMesh = child->AddComponent<MeshComponent>();
							childMesh.path = filePath;
							childMesh.model = Model{};
							childMesh.model.meshes.push_back(loadedModel.meshes[meshIndex]);
							childMesh.model.syndraTextures = loadedModel.syndraTextures;
							scene->SetParent(*child, entity);
						}
					}
					else
					{
						tag = filePath;
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

