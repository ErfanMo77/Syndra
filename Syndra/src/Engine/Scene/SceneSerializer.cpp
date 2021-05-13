#include "lpch.h"
#include "Engine/Scene/SceneSerializer.h"

#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Components.h"
#include "Engine/Utils/PlatformUtils.h"

#include <fstream>
#include <filesystem>
#include <yaml-cpp/yaml.h>

namespace YAML {

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			node.SetStyle(EmitterStyle::Flow);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

}
namespace Syndra {

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	YAML::Emitter& operator<<(YAML::Emitter& out, const std::unordered_map<uint32_t, Ref<Texture2D>>& textures)
	{

		for (auto&& [binding, texture] : textures)
		{
			std::string path = "";
			if (texture) {
				path = texture->GetPath();
			}
			out << YAML::Flow;
			out << YAML::BeginMap;
			out << YAML::Key << "binding" << YAML::Value << binding;
			out << YAML::Key << "path" << YAML::Value << path << YAML::EndMap;
		}
		return out;
	}

	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		: m_Scene(scene)
	{
		m_Shaders = scene->GetShaderLibrary();
	}

	static void SerializeEntity(YAML::Emitter& out, Entity entity)
	{
		out << YAML::BeginMap; // Entity
		out << YAML::Key << "Entity" << YAML::Value << (uint32_t)entity;

		if (entity.HasComponent<TagComponent>())
		{
			out << YAML::Key << "TagComponent";
			out << YAML::BeginMap; // TagComponent

			auto& tag = entity.GetComponent<TagComponent>().Tag;
			out << YAML::Key << "Tag" << YAML::Value << tag;

			out << YAML::EndMap; // TagComponent
		}

		if (entity.HasComponent<TransformComponent>())
		{
			out << YAML::Key << "TransformComponent";
			out << YAML::BeginMap; // TransformComponent

			auto& tc = entity.GetComponent<TransformComponent>();
			out << YAML::Key << "Translation" << YAML::Value << tc.Translation;
			out << YAML::Key << "Rotation" << YAML::Value << tc.Rotation;
			out << YAML::Key << "Scale" << YAML::Value << tc.Scale;

			out << YAML::EndMap; // TransformComponent
		}

		if (entity.HasComponent<CameraComponent>())
		{
			out << YAML::Key << "CameraComponent";
			out << YAML::BeginMap; // CameraComponent

			auto& cameraComponent = entity.GetComponent<CameraComponent>();
			auto& camera = cameraComponent.Camera;

			out << YAML::Key << "Camera" << YAML::Value;
			out << YAML::BeginMap; // Camera
			out << YAML::Key << "ProjectionType" << YAML::Value << (int)camera.GetProjectionType();
			out << YAML::Key << "PerspectiveFOV" << YAML::Value << camera.GetPerspectiveVerticalFOV();
			out << YAML::Key << "PerspectiveNear" << YAML::Value << camera.GetPerspectiveNearClip();
			out << YAML::Key << "PerspectiveFar" << YAML::Value << camera.GetPerspectiveFarClip();
			out << YAML::Key << "OrthographicSize" << YAML::Value << camera.GetOrthographicSize();
			out << YAML::Key << "OrthographicNear" << YAML::Value << camera.GetOrthographicNearClip();
			out << YAML::Key << "OrthographicFar" << YAML::Value << camera.GetOrthographicFarClip();
			out << YAML::EndMap; // Camera

			out << YAML::Key << "Primary" << YAML::Value << cameraComponent.Primary;
			out << YAML::Key << "FixedAspectRatio" << YAML::Value << cameraComponent.FixedAspectRatio;

			out << YAML::EndMap; // CameraComponent
		}

		if (entity.HasComponent<MeshComponent>())
		{
			out << YAML::Key << "MeshComponent";
			out << YAML::BeginMap; // MeshComponent

			auto& path = entity.GetComponent<MeshComponent>().path;
			out << YAML::Key << "Path" << YAML::Value << path;

			out << YAML::EndMap; // MeshComponent
		}

		if (entity.HasComponent<LightComponent>())
		{
			out << YAML::Key << "PointLightComponent";
			out << YAML::BeginMap; // PointLightComponent

			auto& pl = entity.GetComponent<LightComponent>();
			//out << YAML::Key << "color" << YAML::Value << pl.color;
			//out << YAML::Key << "distance" << YAML::Value << pl.dist;
			out << YAML::EndMap; // PointLightComponent
		}

		if (entity.HasComponent<MaterialComponent>())
		{
			out << YAML::Key << "MaterialComponent";
			out << YAML::BeginMap; // MaterialComponent
			auto& material = entity.GetComponent<MaterialComponent>();
			auto& shader = material.m_Shader;
			out << YAML::Key << "shader" << YAML::Value << shader->GetName();

			out << YAML::Key << "Textures" << YAML::Value << YAML::BeginSeq;
			out << material.material->GetTextures();
			out << YAML::EndSeq;

			out << YAML::EndMap; // MaterialComponent
		}

		out << YAML::EndMap; // Entity
	}

	void SceneSerializer::Serialize(const std::string& filepath)
	{
		YAML::Emitter out;

		auto nameWithPost = filepath.substr(filepath.find_last_of("\\")+1);
		auto name = nameWithPost.substr(0,nameWithPost.find("."));

		out << YAML::BeginMap;
		out << YAML::Key << "Scene" << YAML::Value << name;

		//camera
		out << YAML::Key << "Camera"   <<   YAML::Value << YAML::BeginMap;
		out << YAML::Key << "Yaw"      <<   YAML::Value << m_Scene->m_Camera->GetYaw();
		out << YAML::Key << "Pitch"    <<   YAML::Value << m_Scene->m_Camera->GetPitch();
		out << YAML::Key << "distance" <<   YAML::Value << m_Scene->m_Camera->GetDistance();
		out << YAML::Key << "FOV"      <<   YAML::Value << m_Scene->m_Camera->GetFOV();
		out << YAML::Key << "Near"     <<   YAML::Value << m_Scene->m_Camera->GetNear();
		out << YAML::Key << "Far"      <<   YAML::Value << m_Scene->m_Camera->GetFar();
		out << YAML::EndMap; // Camera

		out << YAML::Key << "Entities" << YAML::Value << YAML::BeginSeq;
		for (auto& entity : m_Scene->m_Entities)
		{
			SerializeEntity(out, *entity);
		}
		out << YAML::EndSeq;
		out << YAML::EndMap;

		std::ofstream fout(filepath);
		fout << out.c_str();
	}

	bool SceneSerializer::Deserialize(const std::string& filepath)
	{
		YAML::Node data = YAML::LoadFile(filepath);
		if (!data["Scene"])
			return false;

		std::string sceneName = data["Scene"].as<std::string>();
		m_Scene->m_Name = sceneName;
		SN_CORE_TRACE("Deserializing scene '{0}'", sceneName);

		auto camera = data["Camera"];
		auto yaw = camera["Yaw"].as<float>();
		auto pitch = camera["Pitch"].as<float>();
		auto distance = camera["distance"].as<float>();
		auto fov = camera["FOV"].as <float> ();
		auto nearClip = camera["Near"].as<float>();
		auto farClip = camera["Far"].as<float>();

		m_Scene->m_Camera->SetFarClip(farClip);
		m_Scene->m_Camera->SetNearClip(nearClip);
		m_Scene->m_Camera->SetFov(fov);
		m_Scene->m_Camera->SetDistance(distance);
		m_Scene->m_Camera->SetYawPitch(yaw,pitch);


		auto entities = data["Entities"];
		if (entities)
		{
			for (auto entity : entities)
			{
				uint64_t uuid = entity["Entity"].as<uint64_t>();

				std::string name;
				auto tagComponent = entity["TagComponent"];
				if (tagComponent)
					name = tagComponent["Tag"].as<std::string>();

				SN_CORE_TRACE("Deserialized entity with ID = {0}, name = {1}", uuid, name);

				auto deserializedEntity = m_Scene->CreateEntity(name);

				auto transformComponent = entity["TransformComponent"];
				if (transformComponent)
				{
					// Entities always have transforms
					auto& tc = deserializedEntity->GetComponent<TransformComponent>();
					tc.Translation = transformComponent["Translation"].as<glm::vec3>();
					tc.Rotation = transformComponent["Rotation"].as<glm::vec3>();
					tc.Scale = transformComponent["Scale"].as<glm::vec3>();
				}

				auto cameraComponent = entity["CameraComponent"];
				if (cameraComponent)
				{
					auto& cc = deserializedEntity->AddComponent<CameraComponent>();

					auto cameraProps = cameraComponent["Camera"];
					cc.Camera.SetProjectionType((SceneCamera::ProjectionType)cameraProps["ProjectionType"].as<int>());

					cc.Camera.SetPerspectiveVerticalFOV(cameraProps["PerspectiveFOV"].as<float>());
					cc.Camera.SetPerspectiveNearClip(cameraProps["PerspectiveNear"].as<float>());
					cc.Camera.SetPerspectiveFarClip(cameraProps["PerspectiveFar"].as<float>());

					cc.Camera.SetOrthographicSize(cameraProps["OrthographicSize"].as<float>());
					cc.Camera.SetOrthographicNearClip(cameraProps["OrthographicNear"].as<float>());
					cc.Camera.SetOrthographicFarClip(cameraProps["OrthographicFar"].as<float>());

					cc.Primary = cameraComponent["Primary"].as<bool>();
					cc.FixedAspectRatio = cameraComponent["FixedAspectRatio"].as<bool>();
				}

				auto meshComponent = entity["MeshComponent"];
				if (meshComponent)
				{
					auto dir = std::filesystem::current_path();
					auto& mc = deserializedEntity->AddComponent<MeshComponent>();
					mc.path = meshComponent["Path"].as<std::string>();
					auto filepath = mc.path;
					if (mc.path.find("\\") == 0) {
						filepath = dir.string() + mc.path;
					}
					if (!filepath.empty())
						mc.model = Model(filepath);
				}

				auto pointLightComponent = entity["PointLightComponent"];
				if (pointLightComponent)
				{
					// Entities always have transforms
					auto& pl = deserializedEntity->AddComponent<LightComponent>();
					pl.light->SetColor(pointLightComponent["color"].as<glm::vec4>());
					// = pointLightComponent["distance"].as<float>();
				}

				auto materialComponent = entity["MaterialComponent"];
				if (materialComponent)
				{
					auto dir = std::filesystem::current_path();
					auto shaderName = materialComponent["shader"].as<std::string>();
					auto shader = m_Shaders.Get(shaderName);

					auto material = Material::Create(shader);
					auto& materialTextures = material->GetTextures();

					auto textures = materialComponent["Textures"];
					if (textures) {
						for (auto texture : textures)
						{
							auto binding = texture["binding"].as<uint32_t>();
							auto texturePath = texture["path"].as<std::string>();
							if (!texturePath.empty()) {
								materialTextures[binding] = Texture2D::Create(texturePath);
							}
						}
					}

					deserializedEntity->AddComponent<MaterialComponent>(material,shader);

					//mc.path = materialComponent["Path"].as<std::string>();
					//auto filepath = mc.path;
					//if (mc.path.find("\\") == 0) {
					//	filepath = dir.string() + mc.path;
					//}
					//mc.model = Model(filepath);
				}

			}
		}

		return true;
	}



}