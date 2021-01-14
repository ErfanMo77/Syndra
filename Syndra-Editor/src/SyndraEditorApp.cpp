#include "lpch.h"
#include <Engine.h>
#include "Engine/Core/EntryPoint.h"
#include "EditorLayer.h"

namespace Syndra {

	class SyndraEditorApp : public Application 
	{
	public:
		SyndraEditorApp() 
			:Application("Syndra Editor")
		{
			PushLayer(new EditorLayer());
		}

		~SyndraEditorApp() {

		}

	};

	Application* CreateApplication() {
		return new SyndraEditorApp();
	}

}
