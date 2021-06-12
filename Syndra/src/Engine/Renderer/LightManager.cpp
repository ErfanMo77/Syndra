#include "lpch.h"
#include "Engine/Renderer/LightManager.h"

namespace Syndra {



	LightManager::LightManager(uint32_t binding)
	{
		m_LightBuffer = UniformBuffer::Create(sizeof(m_PointLights) + sizeof(m_SpotLights) + sizeof(m_DirLight), binding);
	}

	void LightManager::IntitializeLights()
	{
		for (auto& pointLight : m_PointLights)
		{
			pointLight.color = glm::vec4(0);
			pointLight.position = glm::vec4(0);
			pointLight.dist = 0.0f;
		}

		for (auto& spot : m_SpotLights)
		{
			spot.color = glm::vec4(0);
			spot.position = glm::vec4(0);
			spot.direction = glm::vec4(0);
			spot.innerCutOff = glm::cos(glm::radians(12.5f));
			spot.outerCutOff = glm::cos(glm::radians(15.0f));
		}

		m_DirLight.color = glm::vec4(0);
		m_DirLight.position = glm::vec4(0);
		m_DirLight.direction = glm::vec4(0);
	}

	void LightManager::UpdateBuffer()
	{
		m_LightBuffer->SetData(&m_PointLights, sizeof(m_PointLights));
		m_LightBuffer->SetData(&m_SpotLights, sizeof(m_SpotLights), sizeof(m_PointLights));
		m_LightBuffer->SetData(&m_DirLight, sizeof(m_DirLight), sizeof(m_PointLights) + sizeof(m_SpotLights));
	}

	void LightManager::UpdateDirLight(DirectionalLight* dl, const glm::vec3& position)
	{
		m_DirLight.color = glm::vec4(dl->GetColor(), 0) * dl->GetIntensity();
		m_DirLight.direction = glm::vec4(dl->GetDirection(), 0);
		m_DirLight.position = glm::vec4(position, 0);
	}

	void LightManager::UpdatePointLights(PointLight* pl, const glm::vec3& position, uint32_t index)
	{
		m_PointLights[index].color = glm::vec4(pl->GetColor(), 1) * pl->GetIntensity();
		m_PointLights[index].position = glm::vec4(position, 1);
		m_PointLights[index].dist = pl->GetRange();
	}

	void LightManager::UpdateSpotLights(SpotLight* sl, const glm::vec3& position, uint32_t index)
	{
		m_SpotLights[index].color = glm::vec4(sl->GetColor(), 1) * sl->GetIntensity();
		m_SpotLights[index].position = glm::vec4(position, 1);
		m_SpotLights[index].direction = glm::vec4(sl->GetDirection(), 0);
		m_SpotLights[index].innerCutOff = glm::cos(glm::radians(sl->GetInnerCutOff()));
		m_SpotLights[index].outerCutOff = glm::cos(glm::radians(sl->GetOuterCutOff()));
	}

}
