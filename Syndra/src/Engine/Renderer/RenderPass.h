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

		uint32_t GetFrameBufferTextureID(uint32_t slot=0);

		void BindTargetFrameBuffer();
		void UnbindTargetFrameBuffer();

		static Ref<RenderPass> Create(const RenderPassSpecification& spec);
		const RenderPassSpecification& GetSpecification() const { return m_Specification; }
	private:
		RenderPassSpecification m_Specification;
	};

}