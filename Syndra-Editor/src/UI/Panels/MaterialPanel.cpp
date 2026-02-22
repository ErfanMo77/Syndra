#include "lpch.h"
#include "MaterialPanel.h"
#include "Engine/Utils/PlatformUtils.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Scene/Scene.h"


namespace Syndra {

	MaterialPanel::MaterialPanel()
	{
		m_EmptyTexture = Texture2D::Create("assets/Models/cube/default.png");
		m_TextureId = ImGuiLayer::GetTextureID(m_EmptyTexture->GetRendererID());
	}

	void MaterialPanel::DrawMaterial(Entity& entity)
	{
		static bool MaterialRemoved = false;
		if (UI::DrawComponent<MaterialComponent>(ICON_FA_PAINT_BRUSH" Material", entity, true, &MaterialRemoved))
		{
			auto& component = entity.GetComponent<MaterialComponent>();
			Scene* scene = Entity::s_Scene;
			if (scene == nullptr)
			{
				ImGui::TextDisabled("Scene context is unavailable.");
				ImGui::TreePop();
				return;
			}

			auto material = scene->GetMaterial(component.MaterialId);

			ImGui::Columns(2);
			ImGui::SetColumnWidth(0, 80);
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{ 0, 0 });
			ImGui::Text("Material");

			ImGui::PopStyleVar();
			ImGui::NextColumn();
			ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
			const auto materialList = scene->GetMaterialList();
			std::string selectedLabel = "Material";
			for (const auto& [materialId, materialName] : materialList)
			{
				if (materialId == component.MaterialId)
				{
					selectedLabel = materialName + "##" + std::to_string(materialId);
					break;
				}
			}
			if (ImGui::BeginCombo("##MaterialSelector", selectedLabel.c_str()))
			{
				for (const auto& [materialId, materialName] : materialList)
				{
					const bool isSelected = materialId == component.MaterialId;
					const std::string label = materialName + "##" + std::to_string(materialId);
					if (ImGui::Selectable(label.c_str(), isSelected))
					{
						scene->AssignMaterial(entity, materialId);
						component.MaterialId = materialId;
						material = scene->GetMaterial(component.MaterialId);
					}
					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
			ImGui::PopItemWidth();
			ImGui::Columns(1);
			if (!material)
			{
				ImGui::TextDisabled("Material %llu is missing.", static_cast<unsigned long long>(component.MaterialId));
				ImGui::TreePop();
				return;
			}
			ImGui::Text("Users: %u", scene->GetMaterialUsageCount(component.MaterialId));
			if (ImGui::Button("Make Unique"))
			{
				const uint64_t uniqueMaterialId = scene->CloneMaterial(component.MaterialId, "Material Instance");
				if (uniqueMaterialId != 0)
				{
					scene->AssignMaterial(entity, uniqueMaterialId);
					component.MaterialId = uniqueMaterialId;
					material = scene->GetMaterial(component.MaterialId);
				}
			}

			ImGui::Separator();

			std::vector<Sampler>& samplers = material->GetSamplers();
			auto& materialTextures = material->GetTextures();
			const auto& buffer = material->GetCBuffer();

			float tiling = buffer.tiling;
			if (UI::DragFloat("Tiling", &tiling, 0.05f, 0.001f, 100)) {
				material->Set("tiling", tiling);
			}

			for (auto& sampler : samplers)
			{
				if (sampler.binding < 5) {
					ImGui::PushID(sampler.name.c_str());
					ImGui::Separator();
					int frame_padding = -1 + 0;                              // -1 == uses default padding (style.FramePadding)
					ImVec2 size = ImVec2(64.0f, 64.0f);                      // Size of the image we want to make visible
					ImGui::Checkbox("Use", &sampler.isUsed);

					ImGui::SameLine();
					ImGui::Text(sampler.name.c_str());

					m_TextureId = ImGuiLayer::GetTextureID(m_EmptyTexture->GetRendererID());
					auto& texture = material->GetTexture(sampler);
					if (texture)
					{
						m_TextureId = ImGuiLayer::GetTextureID(texture->GetRendererID());
					}

					if (ImGui::ImageButton("##TextureButton", m_TextureId, size, ImVec2{ 0, 1 }, ImVec2{ 1, 0 })) {

						auto path = FileDialogs::OpenFile("Syndra Texture (*.*)\0*.*\0");
						if (path) {
							//Add texture as sRGB color space if it is binded to 0 (diffuse texture binding)
							materialTextures[sampler.binding] = Texture2D::Create(*path);
						}
					}

					//Albedo color
					if (sampler.binding == 0) {
						glm::vec4 color = buffer.material.color;
						if (ImGui::ColorEdit4("Albedo", glm::value_ptr(color), ImGuiColorEditFlags_NoInputs)) {
							material->Set("push.material.color", color);
						}
						material->Set("HasAlbedoMap", sampler.isUsed);
					}
					//metal factor
					if (sampler.binding == 1) {
						float metal = buffer.material.MetallicFactor;
						if (UI::SliderFloat("Metallic", &metal, 0.0f, 1.0f)) {
							material->Set("push.material.MetallicFactor", metal);
						}
						material->Set("HasMetallicMap", sampler.isUsed);
					}
					//Use Normal map
					if (sampler.binding == 2) {
						material->Set("HasNormalMap", sampler.isUsed);
					}
					//Roughness factor
					if (sampler.binding == 3) {
						float roughness = buffer.material.RoughnessFactor;
						if (UI::SliderFloat("Roughness", &roughness, 0.0f, 1.0f)) {
							material->Set("push.material.RoughnessFactor", roughness);
						}
						material->Set("HasRoughnessMap", sampler.isUsed);
					}
					//Ambient Occlusion factor
					if (sampler.binding == 4) {
						float AO = buffer.material.AO;
						if (UI::SliderFloat("Ambient Occlusion", &AO, 0.0f, 1.0f)) {
							material->Set("push.material.AO", AO);
						}
						material->Set("HasAOMap", sampler.isUsed);
					}
					ImGui::PopID();
				}
			}
			ImGui::TreePop();
		}

		if (MaterialRemoved) {
			Scene* scene = Entity::s_Scene;
			entity.RemoveComponent<MaterialComponent>();
			if (scene)
				scene->PruneUnusedMaterials();
			MaterialRemoved = false;
		}
	}

}

