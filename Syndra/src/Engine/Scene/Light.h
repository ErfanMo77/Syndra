#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace Syndra {

	class Light {

	public:
		Light() = default;
		Light(const glm::vec3& color, float intensity) : m_Color(color), m_Intensity(intensity) {}
		virtual ~Light() = default;

		void SetColor(const glm::vec3& color) { m_Color = color; }
		glm::vec3 GetColor() const { return m_Color; }

		void SetIntensity(float intensity) { m_Intensity = intensity; }
		float GetIntensity() const { return m_Intensity; }

	private:
		glm::vec3 m_Color = { 1.0,1.0,1.0 };
		float m_Intensity = 1.0f;
	};


	class DirectionalLight : public Light {
	public:
		DirectionalLight() = default;
		DirectionalLight(const glm::vec3& color) : Light(color,1.0f) {}
		DirectionalLight(const glm::vec3& color, float intensity) : Light(color, intensity){}
		DirectionalLight(const glm::vec3& color, float intensity, const glm::vec3& dir)
			:Light(color, intensity), m_Direction(dir){}

		DirectionalLight(const DirectionalLight& light) {
			this->SetColor(light.GetColor());
			this->SetIntensity(light.GetIntensity());
		}

		virtual ~DirectionalLight() = default;


		void SetDirection(const glm::vec3& dir) { m_Direction = dir; }
		glm::vec3 GetDirection() const { return m_Direction; }

	private:
		glm::vec3 m_Direction = {-4.0f,-6.28f,0.0f};
	};


	class PointLight : public Light {
	public:
		PointLight() = default;
		PointLight(const glm::vec3& color) : Light(color,1.0f) {}
		PointLight(const glm::vec3& color, float intensity) : Light(color, intensity) {}
		PointLight(const glm::vec3 & color, const glm::vec3& pos)
			:Light(color,1.0f), m_Position(pos) {}
		PointLight(const glm::vec3& color, float intensity, const glm::vec3& pos, float range)
			:Light(color, intensity), m_Position(pos), m_Range(range) {}

		PointLight(const PointLight& light) {
			this->SetColor(light.GetColor());
			this->SetIntensity(light.GetIntensity());
		}

		virtual ~PointLight() = default;

		void SetRange(float range) { m_Range = range; }
		float GetRange() const { return m_Range; }

		void SetPosition(const glm::vec3& pos) { m_Position = pos; }
		glm::vec3 GetPosition() const { return m_Position; }

	private:
		glm::vec3 m_Position = { 0.0f,0.0f,0.0f };
		float m_Range = 10;
	};


	class SpotLight : public Light {
	public:
		SpotLight() = default;
		SpotLight(const glm::vec3 & color) : Light(color,1.0f) {}
		SpotLight(const glm::vec3& color, float intensity) : Light(color, intensity) {}
		SpotLight(const glm::vec3& color, float intensity, const glm::vec3& pos, const glm::vec3& dir, float cutOff, float outerCutOff)
			:Light(color, intensity), m_Position(pos), m_Direction(dir), m_CutOff(cutOff), m_OuterCutOff(outerCutOff) {}

		SpotLight(const SpotLight& light) {
			this->SetColor(light.GetColor());
			this->SetIntensity(light.GetIntensity());
		}

		virtual ~SpotLight() = default;

		void SetPosition(const glm::vec3& pos) { m_Position = pos; }
		glm::vec3 GetPosition() const { return m_Position; }

		void SetDirection(const glm::vec3& dir) { m_Direction = dir; }
		glm::vec3 GetDirection() const { return m_Direction; }

		void SetCutOff(float cutOff, float outerCutOff) { m_CutOff = cutOff; m_OuterCutOff = outerCutOff; }
		float GetInnerCutOff() const { return m_CutOff; }
		float GetOuterCutOff() const { return m_OuterCutOff; }

	private:
		glm::vec3 m_Position = { 0.0f,0.0f,0.0f };
		glm::vec3 m_Direction = { -1.0f,0.0f,0.0f };

		float m_CutOff = 12.5f;
		float m_OuterCutOff = 15.0f;
	};

}