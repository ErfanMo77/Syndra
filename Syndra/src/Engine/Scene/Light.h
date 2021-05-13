#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


namespace Syndra {

	class Light {

	public:
		Light() = default;
		Light(const glm::vec3& color) : m_Color(color) {}
		virtual ~Light() = default;

		void SetColor(const glm::vec3& color) { m_Color = color; }
		glm::vec3 GetColor() const { return m_Color; }

	private:
		glm::vec3 m_Color = { 1.0,1.0,1.0 };
	};


	class DirectionalLight : public Light {
	public:
		DirectionalLight() = default;
		DirectionalLight(const glm::vec3& color) : Light(color) {}
		DirectionalLight(const glm::vec3& color,const glm::vec3& dir)
			:Light(color), m_Direction(dir) {}

		virtual ~DirectionalLight() = default;


		void SetDirection(const glm::vec3& dir) { m_Direction = dir; }
		glm::vec3 GetDirection() const { return m_Direction; }

	private:
		glm::vec3 m_Direction = {0.5f,0.7f,0.0f};
	};


	class PointLight : public Light {
	public:
		PointLight() = default;
		PointLight(const glm::vec3 & color) : Light(color) {}
		PointLight(const glm::vec3 & color, const glm::vec3& pos)
			:Light(color), m_Position(pos) {}
		PointLight(const glm::vec3& color, const glm::vec3& pos, float range)
			:Light(color), m_Position(pos), m_Range(range) {}


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
		SpotLight(const glm::vec3 & color) : Light(color) {}
		SpotLight(const glm::vec3& color, const glm::vec3& pos, const glm::vec3& dir, float cutOff, float outerCutOff)
			:Light(color), m_Position(pos), m_Direction(dir), m_CutOff(cutOff), m_OuterCutOff(outerCutOff) {}

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

		float m_CutOff = 0;
		float m_OuterCutOff = 0;
	};

}