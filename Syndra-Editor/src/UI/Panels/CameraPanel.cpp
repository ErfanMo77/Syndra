#include "lpch.h"
#include "CameraPanel.h"
#include "Engine/Utils/PlatformUtils.h"

namespace Syndra {

	CameraPanel::CameraPanel()
	{
	}

	void CameraPanel::DrawCamera(Entity& entity)
	{
		static bool CameraRemoved = false;
		if (UI::DrawComponent<CameraComponent>(ICON_FA_CAMERA" Camera", entity, true, &CameraRemoved))
		{
			auto& component = entity.GetComponent<CameraComponent>();
			auto& camera = component.Camera;

			ImGui::Checkbox("Primary", &component.Primary);

			const char* projectionTypeStrings[] = { "Perspective", "Orthographic" };
			const char* currentProjectionTypeString = projectionTypeStrings[(int)camera.GetProjectionType()];
			if (ImGui::BeginCombo("Projection", currentProjectionTypeString))
			{
				for (int i = 0; i < 2; i++)
				{
					bool isSelected = currentProjectionTypeString == projectionTypeStrings[i];
					if (ImGui::Selectable(projectionTypeStrings[i], isSelected))
					{
						currentProjectionTypeString = projectionTypeStrings[i];
						camera.SetProjectionType((SceneCamera::ProjectionType)i);
					}

					if (isSelected)
						ImGui::SetItemDefaultFocus();
				}

				ImGui::EndCombo();
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Perspective)
			{
				float perspectiveVerticalFov = glm::degrees(camera.GetPerspectiveVerticalFOV());
				if (UI::DragFloat("Vertical FOV", &perspectiveVerticalFov, 0.5F, 10, 189.0f))
					camera.SetPerspectiveVerticalFOV(glm::radians(perspectiveVerticalFov));

				float perspectiveNear = camera.GetPerspectiveNearClip();
				if (UI::DragFloat("Near", &perspectiveNear, 0.1f, 0.001f, 100.0f))
					camera.SetPerspectiveNearClip(perspectiveNear);

				float perspectiveFar = camera.GetPerspectiveFarClip();
				if (UI::DragFloat("Far", &perspectiveFar, 0.1f, 100.0f, 10000.0f))
					camera.SetPerspectiveFarClip(perspectiveFar);
			}

			if (camera.GetProjectionType() == SceneCamera::ProjectionType::Orthographic)
			{
				float orthoSize = camera.GetOrthographicSize();
				if (UI::DragFloat("Size", &orthoSize, 0.5f, 0.01f, 100.0f))
					camera.SetOrthographicSize(orthoSize);

				float orthoNear = camera.GetOrthographicNearClip();
				if (UI::DragFloat("Near", &orthoNear, 0.5f, 0.01f, 100.0f))
					camera.SetOrthographicNearClip(orthoNear);

				float orthoFar = camera.GetOrthographicFarClip();
				if (UI::DragFloat("Far", &orthoFar, 0.5f, 0.01f, 100.0f))
					camera.SetOrthographicFarClip(orthoFar);

				ImGui::Checkbox("Fixed Aspect Ratio", &component.FixedAspectRatio);
			}
			ImGui::Separator();
			ImGui::TreePop();
		}
		if (CameraRemoved) {
			entity.RemoveComponent<CameraComponent>();
			CameraRemoved = false;
		}
	}

}