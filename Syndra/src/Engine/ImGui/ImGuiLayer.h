#pragma once

#include "Engine/Core/Layer.h"
#include "Engine/Events/Event.h"
#include "Engine/Events/ApplicationEvent.h"
#include "Engine/Events/MouseEvent.h"
#include "Engine/Events/KeyEvent.h"

#include <imgui.h>
#include <volk.h>

#include <unordered_map>
#include <unordered_set>

namespace Syndra {

	class ImGuiLayer : public Layer
	{
	public:
		ImGuiLayer();
		~ImGuiLayer();

		virtual void OnAttach() override;
		virtual void OnDetach() override;
		virtual void OnImGuiRender() override;

		void Begin();
		void End();
		static ImTextureID GetTextureID(uint32_t rendererID);

		void SetDarkThemeColors();
		void SetBlockEvents(bool blockEvents) { m_BlockEvents = blockEvents; }

		virtual void OnEvent(Event& event) override;

	private:
		enum class Backend
		{
			None = 0,
			OpenGL,
			Vulkan
		};

		ImTextureID ResolveTextureID(uint32_t rendererID);
		void RenderVulkanDrawData(VkCommandBuffer commandBuffer, uint32_t imageIndex);
		void RemoveStaleVulkanTextureDescriptors();
		void RemoveAllVulkanTextureDescriptors();

	private:
		static ImGuiLayer* s_Instance;
		Backend m_Backend = Backend::None;
		bool m_BlockEvents = true;
		uint32_t m_LastVulkanMinImageCount = 0;
		std::unordered_map<uint32_t, VkDescriptorSet> m_VulkanTextureCache;
		std::unordered_set<uint32_t> m_MissingVulkanTextureWarnings;
	};

}
