#pragma once
#include "Engine/Renderer/FrameBuffer.h"

namespace Syndra {

	struct RenderPassSpecification 
	{
		Ref<FrameBuffer> TargetFrameBuffer;
	};

	class RenderPass {

	public:
		RenderPass(const RenderPassSpecification& spec);
		~RenderPass() = default;

		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
		const RenderPassSpecification& GetSpecification() const { return m_Specification; }
	private:
		RenderPassSpecification m_Specification;
	};

}