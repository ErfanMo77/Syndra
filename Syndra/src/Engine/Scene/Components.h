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
			type = LightType::Directional;
			light = CreateRef<DirectionalLight>();
		};

		LightComponent(const LightComponent& other) {
			type = other.type;
			switch (type)
			{
			case Syndra::LightType::Directional:
				light = CreateRef<DirectionalLight>(other.light->GetColor(), other.light->GetIntensity());
				break;
			case Syndra::LightType::Point:
				light = CreateRef<PointLight>(other.light->GetColor(), other.light->GetIntensity());
				break;
			case Syndra::LightType::Spot:
				light = CreateRef<SpotLight>(other.light->GetColor(),other.light->GetIntensity());
				break;
			case Syndra::LightType::Area:
				break;
			default:
				break;
			}
			

		};
	};

	struct MaterialComponent
	{
		Ref<Material> material;
		Ref<Shader> m_Shader;

		MaterialComponent() = default;
		MaterialComponent(const MaterialComponent& material) = default;
		MaterialComponent(const Ref<Material>& material, const Ref<Shader>& shader)
			:material(material), m_Shader(shader)
		{}
		MaterialComponent(Ref<Shader> shader) {
			m_Shader = shader;
			material = Material::Create(shader);
		}
	};

}

