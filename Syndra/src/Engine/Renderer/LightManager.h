#pragma once
#include "Engine/Scene/Light.h"
#include "Engine/Renderer/UniformBuffer.h"

namespace Syndra {

	struct pointLight
	{
		glm::vec4 position;
		glm::vec4 color;
		float dist;
		glm::vec3 dummy;
	};

	struct spotLight {
		glm::vec4 position;
		glm::vec4 color;
		glm::vec4 direction;
		float innerCutOff;
		float outerCutOff;
		glm::vec2 dummy;
	};

	struct directionalLight
	{
		glm::vec4 position;
		glm::vec4 direction;
		glm::vec4 color;
	};

	class LightManager {

	public:
		LightManager(uint32_t binding);

		void IntitializeLights();
		void UpdateBuffer();

		void UpdateDirLight(DirectionalLight* dl, const glm::vec3& position);
		void UpdatePointLights(PointLight* pl, const glm::vec3& position, uint32_t index);
		void UpdateSpotLights(SpotLight* sl, const glm::vec3& position, uint32_t index);

		~LightManager() = default;

	private:
		Ref<UniformBuffer> m_LightBuffer;

		directionalLight m_DirLight;
		pointLight m_PointLights[4];
		spotLight m_SpotLights[4];

	};

}