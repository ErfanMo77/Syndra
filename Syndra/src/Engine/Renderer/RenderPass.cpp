#include "lpch.h"
#include "Engine/Renderer/RenderPass.h"

namespace Syndra {

	RenderPass::RenderPass(const RenderPassSpecification& spec) {
		m_Specification = spec;
	}

	Ref<RenderPass> RenderPass::Create(const RenderPassSpecification& spec)
	{
		return CreateRef<RenderPass>(spec);
	}

}

