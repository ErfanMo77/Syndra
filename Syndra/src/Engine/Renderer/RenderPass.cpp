#include "lpch.h"
#include "Engine/Renderer/RenderPass.h"

namespace Syndra {

	RenderPass::RenderPass(const RenderPassSpecification& spec) {
		m_Specification = spec;
	}

	uint32_t RenderPass::GetFrameBufferTextureID(uint32_t slot)
	{
		return m_Specification.TargetFrameBuffer->GetColorAttachmentRendererID(slot);
	}

	void RenderPass::BindTargetFrameBuffer()
	{
		m_Specification.TargetFrameBuffer->Bind();
	}

	void RenderPass::UnbindTargetFrameBuffer()
	{
		m_Specification.TargetFrameBuffer->Unbind();
	}

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		return CreateRef<RenderPass>(spec);
	}

}

