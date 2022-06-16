#pragma once
#include "Engine/Renderer/PerspectiveCamera.h"
#include "Engine/Scene/Components.h"
#include "Engine/Renderer/Renderer.h"
#include "Engine/Renderer/FrameBuffer.h"
#include "Engine/Renderer/UniformBuffer.h"
#include "Engine/Renderer/Environment.h"
#include "Engine/Renderer/LightManager.h"
#include "Engine/Renderer/RenderPass.h"
#include "Engine/ImGui/IconsFontAwesome5.h"

#include "entt.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Syndra {

	class Entity;
	class Scene;
	
	/* We have two kinds of render pipelines for now:
		Deferred and Forward plus. They inherit this abstract class.*/
	class RenderPipeline {
	
	public:
		virtual ~RenderPipeline() = default;
		
		// Initializing Render Pipeline (scene is required to render mesh entities later)
		virtual void Init(const Ref<Scene>& scene, const ShaderLibrary& shaders, const Ref<Environment>& env) = 0;

		// Rendering the scene
		virtual void Render() = 0;

		// Lighting(in deferred rendering), Post processing and etc.
		virtual void End() = 0;

		//Updating all the lights and their respective shadow maps
		virtual void UpdateLights() = 0;

		//Final colored texture ID required by ImGui
		virtual uint32_t GetFinalTextureID(int index) = 0;

		//Main frame buffer required for resizing UI
		virtual Ref<FrameBuffer> GetMainFrameBuffer() = 0;

		//Resizing the render window
		virtual void OnResize(uint32_t width, uint32_t height) = 0;
	
		// UI related to the render pipeline debuging (e.g. GBuffer)
		virtual void OnImGuiRender(bool* rendererOpen, bool* environmentOpen) = 0;
		
	};
	
}