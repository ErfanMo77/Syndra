#pragma once

namespace Syndra{

	class GraphicsContext
	{
	public:
		virtual ~GraphicsContext() = default;

		virtual void Init() = 0;
		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;
		virtual void SwapBuffers()
		{
			BeginFrame();
			EndFrame();
		}
		virtual void SetVSync(bool) {}
	};

}
