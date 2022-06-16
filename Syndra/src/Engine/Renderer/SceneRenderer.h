#pragma once
#include "DeferredRenderer.h"

namespace Syndra {

	class Entity;
	class Scene;

	class SceneRenderer
	{
	public:
		static void Initialize();
		static void InitializeShaders();
		static void InitializeEnvironment();
		
		static void BeginScene(const PerspectiveCamera& camera);
		static void RenderScene();

		static void EndScene();

		static void Reload(const Ref<Shader>& shader);

		static void OnViewPortResize(uint32_t width, uint32_t height);

		static void OnImGuiRender(bool* rendererOpen, bool* environmentOpen);

		static void SetScene(const Ref<Scene>& scene);
		static void SetEnvironment(const Ref<Environment>& env);

		static uint32_t GetTextureID(int index);

		static FramebufferSpecification GetMainFrameSpec();
		static Ref<FrameBuffer> GetMainFrameBuffer();

		static ShaderLibrary& GetShaderLibrary();


	public:

		struct CameraData
		{
			glm::mat4 ViewProjection;
			glm::vec4 position;
		};

		struct ShadowData {
			glm::mat4 lightViewProj;
			glm::mat4 pointLightViewProj[4][6];
		};

		struct SceneData
		{
			//Scene object
			Ref<Scene> scene;
			CameraData CameraBuffer;
			//render pipeline
			Ref<RenderPipeline> renderPipeline;

			Ref<Environment> environment;
			Ref<UniformBuffer> CameraUniformBuffer;
			//shaders
			ShaderLibrary shaders;
			Ref<Shader> main;
		};

	};
}