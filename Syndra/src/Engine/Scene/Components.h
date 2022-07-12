#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>

#include "Engine/Scene/SceneCamera.h"
#include "Engine/Renderer/Model.h"
#include "Engine/Renderer/Material.h"
#include "Engine/Scene/Light.h"

namespace Syndra {

	struct TagComponent
	{
		std::string Tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& tag)
			: Tag(tag) {}
	};

	struct TransformComponent
	{
		glm::vec3 Translation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Rotation = { 0.0f, 0.0f, 0.0f };
		glm::vec3 Scale = { 1.0f, 1.0f, 1.0f };

		TransformComponent() = default;
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& translation)
			: Translation(translation) {}

		glm::mat4 GetTransform() const
		{
			glm::mat4 rotation = glm::toMat4(glm::quat(Rotation));

			return glm::translate(glm::mat4(1.0f), Translation)
				* rotation
				* glm::scale(glm::mat4(1.0f), Scale);
		}
	};

	struct MeshComponent {

		Model model;
		std::string path;

		MeshComponent() = default;
		MeshComponent(const MeshComponent&) = default;
		MeshComponent(std::string& path)
			:path(path), model(path){}
	};

	struct CameraComponent
	{
		SceneCamera Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;

		CameraComponent() = default;
		CameraComponent(const CameraComponent&) = default;
	};


	enum class LightType
	{
		Directional,
		Point,
		Spot,
		Area
	};

	std::string LightTypeToLightName(LightType type);

	struct LightComponent
	{
		LightType type;
		Ref<Light> light;

		LightComponent() {
			type = LightType::Point;
			light = CreateRef<PointLight>();
		}

		LightComponent(LightType otherType, Ref<Light>& otherLight)
		{
			type = otherType;
			light = otherLight;
		}

		LightComponent(const LightComponent& other) {
			this->type = other.type;
			if (type == LightType::Directional) {
				auto p = reinterpret_cast<DirectionalLight*>(other.light.get());
				this->light = CreateRef<DirectionalLight>(p->GetColor(), p->GetIntensity(), p->GetDirection());
			}
			if (type == LightType::Point) {
				auto p = reinterpret_cast<PointLight*>(other.light.get());
				this->light = CreateRef<PointLight>(p->GetColor(), p->GetIntensity(),p->GetPosition(), p->GetRange());
			}
			if (type == LightType::Spot) {
				auto p = reinterpret_cast<SpotLight*>(other.light.get());
				this->light = CreateRef<SpotLight>(p->GetColor(), p->GetIntensity());
			}
		}
	};

	struct MaterialComponent
	{
		Material m_Material;

		MaterialComponent() = default;
		MaterialComponent(const MaterialComponent& material) = default;
		MaterialComponent(const Ref<Material>& material)
			:m_Material(*material)
		{}
		MaterialComponent(Ref<Shader> shader) {
			m_Material = *Material::Create(shader);
		}
	};

}

