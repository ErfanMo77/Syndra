#pragma once

#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>

#include "../UI.h"

namespace Syndra {

	class MaterialPanel {

	public:
		MaterialPanel();
		~MaterialPanel() = default;

		void DrawMaterial(Entity& entity);

	private:
		Ref<Texture2D> m_EmptyTexture;
		ImTextureID m_TextureId;
	};

}