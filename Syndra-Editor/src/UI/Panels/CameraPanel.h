#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "../UI.h"

namespace Syndra {

	class CameraPanel {

	public:
		CameraPanel();
		~CameraPanel() = default;

		void DrawCamera(Entity& entity);

	private:
		Ref<Texture2D> m_EmptyTexture;
		ImTextureID m_TextureId;
	};

}