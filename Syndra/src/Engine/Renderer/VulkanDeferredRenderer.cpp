#include "lpch.h"

#include "Engine/Renderer/VulkanDeferredRenderer.h"

#include "Engine/Core/Instrument.h"
#include "Engine/ImGui/ImGuiLayer.h"
#include "Engine/Scene/Entity.h"
#include "Engine/Scene/Scene.h"
#include "Engine/Utils/PlatformUtils.h"
#include "imgui.h"

#include <algorithm>
#include <array>
#include <filesystem>

namespace Syndra {

	namespace {

		struct FrustumPlane
		{
			glm::vec3 Normal = glm::vec3(0.0f);
			float Distance = 0.0f;
		};

		using FrustumPlanes = std::array<FrustumPlane, 6>;

		enum FrustumPlaneIndex : size_t
		{
			Left = 0,
			Right,
			Bottom,
			Top,
			Near,
			Far
		};

		glm::mat4 ConvertOpenGLClipToVulkanClip(const glm::mat4& matrix)
		{
			glm::mat4 clip = glm::mat4(1.0f);
			clip[1][1] = -1.0f;
			clip[2][2] = 0.5f;
			clip[3][2] = 0.5f;
			return clip * matrix;
		}

		FrustumPlane NormalizePlane(const glm::vec4& plane)
		{
			FrustumPlane result{};
			const glm::vec3 normal = glm::vec3(plane);
			const float length = glm::length(normal);
			if (length <= 1e-6f)
				return result;

			result.Normal = normal / length;
			result.Distance = plane.w / length;
			return result;
		}

		FrustumPlanes BuildFrustumPlanes(const glm::mat4& viewProjection)
		{
			FrustumPlanes planes{};

			planes[FrustumPlaneIndex::Left] = NormalizePlane(glm::vec4(
				viewProjection[0][3] + viewProjection[0][0],
				viewProjection[1][3] + viewProjection[1][0],
				viewProjection[2][3] + viewProjection[2][0],
				viewProjection[3][3] + viewProjection[3][0]));
			planes[FrustumPlaneIndex::Right] = NormalizePlane(glm::vec4(
				viewProjection[0][3] - viewProjection[0][0],
				viewProjection[1][3] - viewProjection[1][0],
				viewProjection[2][3] - viewProjection[2][0],
				viewProjection[3][3] - viewProjection[3][0]));
			planes[FrustumPlaneIndex::Bottom] = NormalizePlane(glm::vec4(
				viewProjection[0][3] + viewProjection[0][1],
				viewProjection[1][3] + viewProjection[1][1],
				viewProjection[2][3] + viewProjection[2][1],
				viewProjection[3][3] + viewProjection[3][1]));
			planes[FrustumPlaneIndex::Top] = NormalizePlane(glm::vec4(
				viewProjection[0][3] - viewProjection[0][1],
				viewProjection[1][3] - viewProjection[1][1],
				viewProjection[2][3] - viewProjection[2][1],
				viewProjection[3][3] - viewProjection[3][1]));
			planes[FrustumPlaneIndex::Near] = NormalizePlane(glm::vec4(
				viewProjection[0][3] + viewProjection[0][2],
				viewProjection[1][3] + viewProjection[1][2],
				viewProjection[2][3] + viewProjection[2][2],
				viewProjection[3][3] + viewProjection[3][2]));
			planes[FrustumPlaneIndex::Far] = NormalizePlane(glm::vec4(
				viewProjection[0][3] - viewProjection[0][2],
				viewProjection[1][3] - viewProjection[1][2],
				viewProjection[2][3] - viewProjection[2][2],
				viewProjection[3][3] - viewProjection[3][2]));

			return planes;
		}

		bool IsSphereInsideFrustum(const FrustumPlanes& planes, const glm::vec3& center, float radius)
		{
			for (const auto& plane : planes)
			{
				const float distanceToPlane = glm::dot(plane.Normal, center) + plane.Distance;
				if (distanceToPlane < -radius)
					return false;
			}

			return true;
		}

		bool ComputeModelBounds(const Model& model, glm::vec3& outMin, glm::vec3& outMax)
		{
			bool hasBounds = false;
			for (const auto& mesh : model.meshes)
			{
				if (!mesh.HasBounds())
					continue;

				if (!hasBounds)
				{
					outMin = mesh.GetBoundsMin();
					outMax = mesh.GetBoundsMax();
					hasBounds = true;
					continue;
				}

				outMin = glm::min(outMin, mesh.GetBoundsMin());
				outMax = glm::max(outMax, mesh.GetBoundsMax());
			}

			return hasBounds;
		}

		float ComputeTransformMaxScale(const glm::mat4& transform)
		{
			const glm::vec3 xAxis = glm::vec3(transform[0]);
			const glm::vec3 yAxis = glm::vec3(transform[1]);
			const glm::vec3 zAxis = glm::vec3(transform[2]);
			return std::max({ glm::length(xAxis), glm::length(yAxis), glm::length(zAxis), 1e-4f });
		}

		bool IsModelVisibleInCameraFrustum(const Model& model, const glm::mat4& worldTransform, const FrustumPlanes& cameraFrustum)
		{
			glm::vec3 localMin(0.0f);
			glm::vec3 localMax(0.0f);
			if (!ComputeModelBounds(model, localMin, localMax))
				return true;

			const glm::vec3 localCenter = (localMin + localMax) * 0.5f;
			const glm::vec3 localExtents = (localMax - localMin) * 0.5f;
			const float localRadius = glm::length(localExtents);
			if (localRadius <= 1e-6f)
				return true;

			const glm::vec3 worldCenter = glm::vec3(worldTransform * glm::vec4(localCenter, 1.0f));
			const float worldRadius = localRadius * ComputeTransformMaxScale(worldTransform);
			return IsSphereInsideFrustum(cameraFrustum, worldCenter, worldRadius);
		}

	}

	static VulkanDeferredRenderer::RenderData r_Data;

	void VulkanDeferredRenderer::Init(const Ref<Scene>& scene, const ShaderLibrary& shaders, const Ref<Environment>&)
	{
		r_Data = {};
		r_Data.scene = scene;
		r_Data.shaders = shaders;

		FramebufferSpecification shadowSpec;
		shadowSpec.Attachments = { FramebufferTextureFormat::DEPTH32 };
		shadowSpec.Width = r_Data.shadowMapSize;
		shadowSpec.Height = r_Data.shadowMapSize;
		shadowSpec.Samples = 1;
		shadowSpec.ClearColor = glm::vec4(1.0f);

		RenderPassSpecification shadowPassSpec;
		shadowPassSpec.TargetFrameBuffer = FrameBuffer::Create(shadowSpec);
		r_Data.shadowPass = RenderPass::Create(shadowPassSpec);

		FramebufferSpecification geometrySpec;
		geometrySpec.Attachments =
		{
			FramebufferTextureFormat::RGBA16F,
			FramebufferTextureFormat::RGBA16F,
			FramebufferTextureFormat::RGBA8,
			FramebufferTextureFormat::RGBA8,
			FramebufferTextureFormat::RED_INTEGER,
			FramebufferTextureFormat::DEPTH24STENCIL8
		};
		geometrySpec.Width = 1280;
		geometrySpec.Height = 720;
		geometrySpec.Samples = 1;
		geometrySpec.ClearColor = glm::vec4(0.02f, 0.02f, 0.03f, 1.0f);

		RenderPassSpecification geometryPassSpec;
		geometryPassSpec.TargetFrameBuffer = FrameBuffer::Create(geometrySpec);
		r_Data.geometryPass = RenderPass::Create(geometryPassSpec);

		FramebufferSpecification lightingSpec;
		lightingSpec.Attachments =
		{
			FramebufferTextureFormat::RGBA8,
			FramebufferTextureFormat::DEPTH24STENCIL8
		};
		lightingSpec.Width = 1280;
		lightingSpec.Height = 720;
		lightingSpec.Samples = 1;
		lightingSpec.ClearColor = glm::vec4(0.08f, 0.08f, 0.1f, 1.0f);

		RenderPassSpecification lightingPassSpec;
		lightingPassSpec.TargetFrameBuffer = FrameBuffer::Create(lightingSpec);
		r_Data.lightingPass = RenderPass::Create(lightingPassSpec);

		FramebufferSpecification aaSpec;
		aaSpec.Attachments = { FramebufferTextureFormat::RGBA8 };
		aaSpec.Width = 1280;
		aaSpec.Height = 720;
		aaSpec.Samples = 1;
		aaSpec.ClearColor = glm::vec4(0.08f, 0.08f, 0.1f, 1.0f);

		RenderPassSpecification aaPassSpec;
		aaPassSpec.TargetFrameBuffer = FrameBuffer::Create(aaSpec);
		r_Data.aaPass = RenderPass::Create(aaPassSpec);

		if (r_Data.shaders.Exists("GeometryPass"))
			r_Data.geometryShader = r_Data.shaders.Get("GeometryPass");
		if (r_Data.shaders.Exists("DeferredLighting"))
			r_Data.lightingShader = r_Data.shaders.Get("DeferredLighting");
		if (r_Data.shaders.Exists("depth"))
			r_Data.shadowShader = r_Data.shaders.Get("depth");
		if (r_Data.shaders.Exists("FXAA"))
			r_Data.fxaaShader = r_Data.shaders.Get("FXAA");

		// Vulkan IBL uses an HDR equirectangular environment texture directly in the lighting pass.
		auto LoadEnvironment = [&](const std::string& environmentPath)
		{
			if (environmentPath.empty())
				return;

			r_Data.environmentMap = Texture2D::CreateHDR(environmentPath, false, true);
		};

		if (r_Data.scene)
		{
			LoadEnvironment(r_Data.scene->m_EnvironmentPath);
			if (!r_Data.environmentMap)
			{
				const std::string defaultEnvironment = "assets/HDRI/san_giuseppe_bridge_4k.hdr";
				if (std::filesystem::exists(defaultEnvironment))
				{
					LoadEnvironment(defaultEnvironment);
					r_Data.scene->m_EnvironmentPath = defaultEnvironment;
				}
			}
		}

		r_Data.lightsUniformBuffer = UniformBuffer::Create(sizeof(RenderData::LightsData), 2);
		r_Data.shadowUniformBuffer = UniformBuffer::Create(sizeof(glm::mat4), 3);
		if (r_Data.shadowUniformBuffer)
			r_Data.shadowUniformBuffer->SetData(glm::value_ptr(r_Data.shadowViewProjection), sizeof(glm::mat4));

		r_Data.screenVao = VertexArray::Create();
		float quad[] = {
			 1.0f,  1.0f, 0.0f,    1.0f, 1.0f,
			 1.0f, -1.0f, 0.0f,    1.0f, 0.0f,
			-1.0f, -1.0f, 0.0f,    0.0f, 0.0f,
			-1.0f,  1.0f, 0.0f,    0.0f, 1.0f
		};
		auto vb = VertexBuffer::Create(quad, sizeof(quad));
		BufferLayout quadLayout = {
			{ ShaderDataType::Float3, "a_pos" },
			{ ShaderDataType::Float2, "a_uv" }
		};
		vb->SetLayout(quadLayout);
		r_Data.screenVao->AddVertexBuffer(vb);

		uint32_t quadIndices[] = {
			0, 3, 1,
			1, 3, 2
		};
		auto ib = IndexBuffer::Create(quadIndices, sizeof(quadIndices) / sizeof(uint32_t));
		r_Data.screenVao->SetIndexBuffer(ib);
	}

	void VulkanDeferredRenderer::Render()
	{
		SN_PROFILE_SCOPE("VulkanDeferredRenderer::Render");
		if (!r_Data.scene || !r_Data.geometryPass)
			return;

		auto view = r_Data.scene->m_Registry.view<TransformComponent, MeshComponent>();
		struct RenderItem
		{
			entt::entity EntityHandle = entt::null;
			MeshComponent* Mesh = nullptr;
			MaterialComponent* Material = nullptr;
			glm::mat4 WorldTransform = glm::mat4(1.0f);
		};

		std::vector<RenderItem> visibleItems;
		visibleItems.reserve(view.size_hint());
		r_Data.visibleMeshEntityCount = 0;
		r_Data.culledMeshEntityCount = 0;

		FrustumPlanes cameraFrustum{};
		if (r_Data.scene->m_Camera)
			cameraFrustum = BuildFrustumPlanes(r_Data.scene->m_Camera->GetViewProjection());

		for (auto ent : view)
		{
			auto& meshComponent = view.get<MeshComponent>(ent);
			if (meshComponent.path.empty())
				continue;

			const glm::mat4 worldTransform = r_Data.scene->GetWorldTransform(Entity{ ent });
			if (r_Data.useFrustumCulling && r_Data.scene->m_Camera &&
				!IsModelVisibleInCameraFrustum(meshComponent.model, worldTransform, cameraFrustum))
			{
				++r_Data.culledMeshEntityCount;
				continue;
			}

			MaterialComponent* materialComponent = r_Data.scene->m_Registry.try_get<MaterialComponent>(ent);
			visibleItems.push_back(RenderItem{ ent, &meshComponent, materialComponent, worldTransform });
		}

		r_Data.visibleMeshEntityCount = static_cast<uint32_t>(visibleItems.size());

		if (r_Data.useShadows && r_Data.shadowPass && r_Data.shadowShader && r_Data.shadowUniformBuffer)
		{
			SN_PROFILE_SCOPE("VulkanDeferredRenderer::ShadowPass");
			glm::vec3 lightDirection = glm::vec3(r_Data.lightsData.dLight.direction);
			if (glm::length(lightDirection) < 0.0001f)
				lightDirection = glm::vec3(-0.6f, -1.0f, -0.35f);
			lightDirection = glm::normalize(lightDirection);

			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
			if (glm::abs(glm::dot(lightDirection, up)) > 0.98f)
				up = glm::vec3(0.0f, 0.0f, 1.0f);

			const glm::vec3 sceneCenter = glm::vec3(0.0f);
			const glm::vec3 lightPosition = sceneCenter - lightDirection * (r_Data.shadowOrthoSize * 0.8f);
			const glm::mat4 lightView = glm::lookAt(lightPosition, sceneCenter, up);
			const float shadowNear = std::max(0.01f, std::min(r_Data.shadowNearPlane, r_Data.shadowFarPlane - 0.01f));
			const float shadowFar = std::max(shadowNear + 0.01f, r_Data.shadowFarPlane);
			const glm::mat4 lightProjection = glm::ortho(
				-r_Data.shadowOrthoSize,
				r_Data.shadowOrthoSize,
				-r_Data.shadowOrthoSize,
				r_Data.shadowOrthoSize,
				shadowNear,
				shadowFar);

			const glm::mat4 lightViewProjection = lightProjection * lightView;
			r_Data.shadowViewProjection = ConvertOpenGLClipToVulkanClip(lightViewProjection);
			r_Data.shadowUniformBuffer->SetData(glm::value_ptr(r_Data.shadowViewProjection), sizeof(glm::mat4));

			r_Data.shadowPass->BindTargetFrameBuffer();
			RenderCommand::SetState(RenderState::DEPTH_TEST, true);
			RenderCommand::SetClearColor(glm::vec4(1.0f));
			RenderCommand::Clear();

			r_Data.shadowShader->Bind();
			for (const auto& item : visibleItems)
			{
				r_Data.shadowShader->SetMat4("push.u_trans", item.WorldTransform);
				r_Data.shadowShader->SetInt("push.id", static_cast<uint32_t>(item.EntityHandle));
				Renderer::Submit(r_Data.shadowShader, item.Mesh->model);
			}
			r_Data.shadowShader->Unbind();
			r_Data.shadowPass->UnbindTargetFrameBuffer();
		}

		{
			SN_PROFILE_SCOPE("VulkanDeferredRenderer::GeometryPass");
			r_Data.geometryPass->BindTargetFrameBuffer();
			RenderCommand::SetState(RenderState::DEPTH_TEST, true);
			RenderCommand::SetClearColor(r_Data.geometryPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor);
			r_Data.geometryPass->GetSpecification().TargetFrameBuffer->ClearAttachment(4, -1);
			RenderCommand::Clear();

			if (r_Data.geometryShader)
				r_Data.geometryShader->Bind();

			for (const auto& item : visibleItems)
			{
				if (item.Material)
				{
					if (r_Data.geometryShader)
					{
						r_Data.geometryShader->SetInt("transform.id", static_cast<uint32_t>(item.EntityHandle));
						r_Data.geometryShader->SetMat4("transform.u_trans", item.WorldTransform);
					}
					Renderer::Submit(item.Material->m_Material, item.Mesh->model);
				}
				else if (r_Data.geometryShader)
				{
					r_Data.geometryShader->SetInt("transform.id", static_cast<uint32_t>(item.EntityHandle));
					r_Data.geometryShader->SetMat4("transform.u_trans", item.WorldTransform);
					r_Data.geometryShader->SetFloat4("push.material.color", glm::vec4(0.8f, 0.8f, 0.8f, 1.0f));
					r_Data.geometryShader->SetFloat("push.material.RoughnessFactor", 0.6f);
					r_Data.geometryShader->SetFloat("push.material.MetallicFactor", 0.0f);
					r_Data.geometryShader->SetFloat("push.material.AO", 1.0f);
					r_Data.geometryShader->SetFloat("push.tiling", 1.0f);
					r_Data.geometryShader->SetInt("push.HasAlbedoMap", 0);
					r_Data.geometryShader->SetInt("push.HasNormalMap", 0);
					r_Data.geometryShader->SetInt("push.HasRoughnessMap", 0);
					r_Data.geometryShader->SetInt("push.HasMetallicMap", 0);
					r_Data.geometryShader->SetInt("push.HasAOMap", 0);
					Renderer::Submit(r_Data.geometryShader, item.Mesh->model);
				}
			}

			if (r_Data.geometryShader)
				r_Data.geometryShader->Unbind();
			r_Data.geometryPass->UnbindTargetFrameBuffer();
		}
	}

	void VulkanDeferredRenderer::End()
	{
		SN_PROFILE_SCOPE("VulkanDeferredRenderer::End");
		if (!r_Data.lightingPass)
			return;

		r_Data.lightingPass->BindTargetFrameBuffer();
		RenderCommand::SetState(RenderState::DEPTH_TEST, false);
		RenderCommand::SetClearColor(r_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor);
		RenderCommand::Clear();

		if (r_Data.lightingShader && r_Data.screenVao)
		{
			SN_PROFILE_SCOPE("VulkanDeferredRenderer::LightingPass");
			r_Data.lightingShader->Bind();
			r_Data.lightingShader->SetFloat("push.exposure", r_Data.exposure);
			r_Data.lightingShader->SetFloat("push.gamma", r_Data.gamma);
			r_Data.lightingShader->SetFloat("push.intensity", r_Data.intensity);
			r_Data.lightingShader->SetInt("push.useShadows", r_Data.useShadows ? 1 : 0);
			const bool hasEnvironment = (r_Data.environmentMap != nullptr);
			r_Data.lightingShader->SetInt("push.useIBL", (r_Data.useIBL && hasEnvironment) ? 1 : 0);
			r_Data.lightingShader->SetFloat("push.iblStrength", r_Data.iblStrength);
			Texture2D::BindTexture(r_Data.geometryPass->GetFrameBufferTextureID(0), 0);
			Texture2D::BindTexture(r_Data.geometryPass->GetFrameBufferTextureID(1), 1);
			Texture2D::BindTexture(r_Data.geometryPass->GetFrameBufferTextureID(2), 2);
			Texture2D::BindTexture(r_Data.geometryPass->GetFrameBufferTextureID(3), 3);
			if (r_Data.useShadows && r_Data.shadowPass)
				Texture2D::BindTexture(r_Data.shadowPass->GetSpecification().TargetFrameBuffer->GetDepthAttachmentRendererID(), 4);
			else
				Texture2D::BindTexture(0, 4);
			if (hasEnvironment)
				r_Data.environmentMap->Bind(5);
			else
				Texture2D::BindTexture(0, 5);
			Renderer::Submit(r_Data.lightingShader, r_Data.screenVao);
			r_Data.lightingShader->Unbind();
		}

		r_Data.lightingPass->UnbindTargetFrameBuffer();

		if (r_Data.useFxaa && r_Data.aaPass && r_Data.fxaaShader && r_Data.screenVao)
		{
			SN_PROFILE_SCOPE("VulkanDeferredRenderer::FXAAPass");
			r_Data.aaPass->BindTargetFrameBuffer();
			RenderCommand::SetState(RenderState::DEPTH_TEST, false);
			RenderCommand::SetClearColor(r_Data.aaPass->GetSpecification().TargetFrameBuffer->GetSpecification().ClearColor);
			RenderCommand::Clear();

			r_Data.fxaaShader->Bind();
			const auto spec = r_Data.aaPass->GetSpecification().TargetFrameBuffer->GetSpecification();
			r_Data.fxaaShader->SetFloat("push.width", static_cast<float>(spec.Width));
			r_Data.fxaaShader->SetFloat("push.height", static_cast<float>(spec.Height));
			Texture2D::BindTexture(r_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetColorAttachmentRendererID(0), 0);
			Renderer::Submit(r_Data.fxaaShader, r_Data.screenVao);
			r_Data.fxaaShader->Unbind();

			r_Data.aaPass->UnbindTargetFrameBuffer();
		}

		Renderer::EndScene();
	}

	void VulkanDeferredRenderer::ShutDown()
	{
		r_Data = {};
	}

	void VulkanDeferredRenderer::UpdateLights()
	{
		SN_PROFILE_SCOPE("VulkanDeferredRenderer::UpdateLights");
		r_Data.directionalLightCount = 0;
		r_Data.pointLightCount = 0;
		r_Data.lightsData = {};

		if (!r_Data.scene)
			return;

		auto viewLights = r_Data.scene->m_Registry.view<TransformComponent, LightComponent>();
		uint32_t pointIndex = 0;
		for (auto ent : viewLights)
		{
			const auto& light = viewLights.get<LightComponent>(ent);
			const glm::vec3 worldTranslation = r_Data.scene->GetWorldTranslation(Entity{ ent });
			if (light.type == LightType::Directional)
			{
				auto directional = reinterpret_cast<DirectionalLight*>(light.light.get());
				if (directional)
				{
					r_Data.lightsData.dLight.direction = glm::vec4(directional->GetDirection(), 0.0f);
					r_Data.lightsData.dLight.color = glm::vec4(directional->GetColor() * directional->GetIntensity(), 0.0f);
				}
				++r_Data.directionalLightCount;
			}
			if (light.type == LightType::Point)
			{
				auto point = reinterpret_cast<PointLight*>(light.light.get());
				if (point && pointIndex < 4)
				{
					r_Data.lightsData.pLight[pointIndex].position = glm::vec4(worldTranslation, 1.0f);
					r_Data.lightsData.pLight[pointIndex].color = glm::vec4(point->GetColor() * point->GetIntensity(), 1.0f);
					++pointIndex;
				}
				++r_Data.pointLightCount;
			}
		}

		if (r_Data.lightsUniformBuffer)
			r_Data.lightsUniformBuffer->SetData(&r_Data.lightsData, sizeof(RenderData::LightsData));
	}

	uint32_t VulkanDeferredRenderer::GetFinalTextureID(int)
	{
		if (r_Data.useFxaa && r_Data.aaPass)
			return r_Data.aaPass->GetSpecification().TargetFrameBuffer->GetColorAttachmentRendererID(0);

		if (!r_Data.lightingPass)
			return 0;

		return r_Data.lightingPass->GetSpecification().TargetFrameBuffer->GetColorAttachmentRendererID(0);
	}

	uint32_t VulkanDeferredRenderer::GetMouseTextureID()
	{
		return 4;
	}

	Ref<FrameBuffer> VulkanDeferredRenderer::GetMainFrameBuffer()
	{
		if (!r_Data.geometryPass)
			return nullptr;

		return r_Data.geometryPass->GetSpecification().TargetFrameBuffer;
	}

	void VulkanDeferredRenderer::OnResize(uint32_t width, uint32_t height)
	{
		if (r_Data.geometryPass)
			r_Data.geometryPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		if (r_Data.lightingPass)
			r_Data.lightingPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
		if (r_Data.aaPass)
			r_Data.aaPass->GetSpecification().TargetFrameBuffer->Resize(width, height);
	}

	void VulkanDeferredRenderer::OnImGuiRender(bool* rendererOpen, bool* environmentOpen)
	{
		if (rendererOpen && *rendererOpen)
		{
			ImGui::Begin(ICON_FA_COGS " Renderer Settings", rendererOpen);
			ImGui::Text("Vulkan Deferred Renderer");
			ImGui::Text("Directional lights: %u", r_Data.directionalLightCount);
			ImGui::Text("Point lights: %u", r_Data.pointLightCount);
			ImGui::Text("Visible mesh entities: %u", r_Data.visibleMeshEntityCount);
			ImGui::Text("Culled mesh entities: %u", r_Data.culledMeshEntityCount);
			ImGui::Checkbox("Shadows", &r_Data.useShadows);
			ImGui::Checkbox("FXAA", &r_Data.useFxaa);
			ImGui::Checkbox("Frustum Culling", &r_Data.useFrustumCulling);
			ImGui::DragFloat("Exposure", &r_Data.exposure, 0.01f, 0.01f, 8.0f);
			ImGui::DragFloat("Gamma", &r_Data.gamma, 0.01f, 0.5f, 4.0f);
			ImGui::DragFloat("Intensity", &r_Data.intensity, 0.01f, 0.0f, 8.0f);
			ImGui::Separator();
			ImGui::Text("Directional Shadow");
			ImGui::DragFloat("Ortho Size", &r_Data.shadowOrthoSize, 0.1f, 5.0f, 200.0f);
			ImGui::DragFloat("Near", &r_Data.shadowNearPlane, 0.1f, 0.1f, 50.0f);
			ImGui::DragFloat("Far", &r_Data.shadowFarPlane, 1.0f, 5.0f, 500.0f);
			ImGui::Separator();
			ImGui::Text("GBuffer");
			if (r_Data.geometryPass)
			{
				const auto spec = r_Data.geometryPass->GetSpecification().TargetFrameBuffer->GetSpecification();
				const float width = ImGui::GetContentRegionAvail().x;
				const float height = (spec.Width > 0) ? (width * (static_cast<float>(spec.Height) / static_cast<float>(spec.Width))) : 0.0f;
				ImGui::Image(ImGuiLayer::GetTextureID(r_Data.geometryPass->GetFrameBufferTextureID(2)), { width, height }, ImVec2{ 0, 0 }, ImVec2{ 1, 1 });
			}
			ImGui::End();
		}

		if (environmentOpen && *environmentOpen)
		{
			ImGui::Begin(ICON_FA_TREE " Environment", environmentOpen);

			ImGui::Checkbox("Enable IBL", &r_Data.useIBL);
			ImGui::DragFloat("IBL Strength", &r_Data.iblStrength, 0.01f, 0.0f, 5.0f);
			ImGui::Separator();

			if (ImGui::Button("Load HDR", { 80.0f, 28.0f }))
			{
				auto path = FileDialogs::OpenFile("HDR (*.hdr)\0*.hdr\0");
				if (path)
				{
					r_Data.environmentMap = Texture2D::CreateHDR(*path, false, true);
					if (r_Data.scene)
						r_Data.scene->m_EnvironmentPath = *path;
				}
			}

			ImGui::SameLine();
			if (ImGui::Button("Clear", { 80.0f, 28.0f }))
			{
				r_Data.environmentMap = nullptr;
				if (r_Data.scene)
					r_Data.scene->m_EnvironmentPath.clear();
			}

			if (r_Data.scene)
				ImGui::TextWrapped("Path: %s", r_Data.scene->m_EnvironmentPath.empty() ? "<none>" : r_Data.scene->m_EnvironmentPath.c_str());

			if (r_Data.environmentMap)
			{
				const float width = ImGui::GetContentRegionAvail().x;
				const float height = width * 0.5f;
				const ImVec2 uv0 = ImVec2{ 0, 1 };
				const ImVec2 uv1 = ImVec2{ 1, 0 };
				ImGui::Image(ImGuiLayer::GetTextureID(r_Data.environmentMap->GetRendererID()), { width, height }, uv0, uv1);
			}

			ImGui::End();
		}
	}

}
