#include "lpch.h"
#include "Platform/OpenGL/OpenGLRendererAPI.h"
#include "glad/glad.h"
#include <vulkan/vulkan.h>
#include <cstdlib>

namespace {

	struct VulkanDriverInfo
	{
		bool available = false;
		uint32_t vendorId = 0;
		uint32_t driverVersion = 0;
		std::string deviceName;
	};

	const char* VulkanVendorName(uint32_t vendorId)
	{
		switch (vendorId)
		{
		case 0x10DE: return "NVIDIA";
		case 0x1002: return "AMD";
		case 0x8086: return "Intel";
		default: return "Unknown";
		}
	}

	std::string FormatVulkanDriverVersion(uint32_t vendorId, uint32_t driverVersion)
	{
		std::ostringstream out;
		if (vendorId == 0x10DE)
		{
			uint32_t major = (driverVersion >> 22) & 0x3ff;
			uint32_t minor = (driverVersion >> 14) & 0x0ff;
			uint32_t patch = (driverVersion >> 6) & 0x0ff;
			uint32_t build = driverVersion & 0x3f;
			out << major << "." << minor << "." << patch << "." << build;
			return out.str();
		}

		if (vendorId == 0x1002)
		{
			uint32_t major = (driverVersion >> 22) & 0x3ff;
			uint32_t minor = (driverVersion >> 12) & 0x3ff;
			uint32_t patch = driverVersion & 0xfff;
			out << major << "." << minor << "." << patch;
			return out.str();
		}

		out << VK_VERSION_MAJOR(driverVersion) << "."
			<< VK_VERSION_MINOR(driverVersion) << "."
			<< VK_VERSION_PATCH(driverVersion);
		return out.str();
	}

	std::string GetVulkanSdkVersion()
	{
		const char* sdkPath = std::getenv("VULKAN_SDK");
		if (!sdkPath || sdkPath[0] == '\0')
			return "unknown";

		std::filesystem::path path(sdkPath);
		path = path.lexically_normal();
		std::string version = path.filename().string();
		if (version.empty() && path.has_parent_path())
			version = path.parent_path().filename().string();
		return version.empty() ? std::string(sdkPath) : version;
	}

	VulkanDriverInfo QueryVulkanDriverInfo()
	{
		VulkanDriverInfo info;

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Syndra";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Syndra";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		VkInstance instance = VK_NULL_HANDLE;
		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
			return info;

		uint32_t deviceCount = 0;
		if (vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr) != VK_SUCCESS || deviceCount == 0)
		{
			vkDestroyInstance(instance, nullptr);
			return info;
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		if (vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data()) == VK_SUCCESS && deviceCount > 0)
		{
			VkPhysicalDeviceProperties props{};
			vkGetPhysicalDeviceProperties(devices[0], &props);
			info.available = true;
			info.vendorId = props.vendorID;
			info.driverVersion = props.driverVersion;
			info.deviceName = props.deviceName;
		}

		vkDestroyInstance(instance, nullptr);
		return info;
	}

	const VulkanDriverInfo& GetCachedVulkanDriverInfo()
	{
		static VulkanDriverInfo info = QueryVulkanDriverInfo();
		return info;
	}

	const std::string& GetCachedVulkanSdkVersion()
	{
		static std::string version = GetVulkanSdkVersion();
		return version;
	}

}

namespace Syndra {

	void OpenGLMessageCallback(
		unsigned source,
		unsigned type,
		unsigned id,
		unsigned severity,
		int length,
		const char* message,
		const void* userParam)
	{
		switch (severity)
		{
		case GL_DEBUG_SEVERITY_HIGH:         SN_CORE_ERROR(message); return;
		case GL_DEBUG_SEVERITY_MEDIUM:       SN_CORE_INFO(message); return;
		case GL_DEBUG_SEVERITY_LOW:          SN_CORE_WARN(message); return;
		case GL_DEBUG_SEVERITY_NOTIFICATION: SN_CORE_TRACE(message); return;
		}

		SN_CORE_ASSERT(false, "Unknown severity level!");
	}

	void OpenGLRendererAPI::Init()
	{
		SN_CORE_WARN("Driver: {0}", glGetString(GL_VENDOR));
		SN_CORE_WARN("Renderer: {0}", glGetString(GL_RENDERER));
		SN_CORE_WARN("Version: {0}", glGetString(GL_VERSION));
		SN_CORE_WARN("Vulkan SDK: {0}", GetCachedVulkanSdkVersion());
		const VulkanDriverInfo& vkInfo = GetCachedVulkanDriverInfo();
		if (vkInfo.available)
		{
			SN_CORE_WARN("Vulkan Driver: {0} ({1}, {2})",
				FormatVulkanDriverVersion(vkInfo.vendorId, vkInfo.driverVersion),
				VulkanVendorName(vkInfo.vendorId),
				vkInfo.deviceName);
		}
		else
		{
			SN_CORE_WARN("Vulkan Driver: unavailable");
		}
#ifdef SN_DEBUG
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback(OpenGLMessageCallback, nullptr);

		glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, NULL, GL_FALSE);
#endif

		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glEnable(GL_BLEND);
		glFrontFace(GL_CW);
		glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
		//glCullFace(GL_FRONT);
		//glEnable(GL_CULL_FACE);
		glEnable(GL_DEPTH_TEST);
	}

	static GLenum RenderStateToGLState(RenderState state)
	{
		switch (state)
		{
		case RenderState::DEPTH_TEST:       return GL_DEPTH_TEST;
		case RenderState::CULL:				return GL_CULL_FACE;
		case RenderState::BLEND:			return GL_BLEND;
		case RenderState::SRGB:             return GL_FRAMEBUFFER_SRGB;
		}

		SN_CORE_ASSERT(false, "Renderstate should be defined!");
		return 0;
	}

	void OpenGLRendererAPI::SetClearColor(const glm::vec4& color)
	{
		glClearColor(color.r, color.g, color.b, color.a);
	}

	void OpenGLRendererAPI::Clear()
	{
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	void OpenGLRendererAPI::DrawIndexed(const Ref<VertexArray>& vertexArray)
	{
		uint32_t count = vertexArray->GetIndexBuffer()->GetCount();
		
		glDrawElements(GL_TRIANGLES, count, GL_UNSIGNED_INT, nullptr);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	void OpenGLRendererAPI::SetViewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height)
	{
		glViewport(x, y, width, height);
	}

	void OpenGLRendererAPI::SetState(RenderState stateID, bool on)
	{
		on ? glEnable(RenderStateToGLState(stateID)) : glDisable(RenderStateToGLState(stateID));
	}

	std::string OpenGLRendererAPI::GetRendererInfo()
	{
		std::string info{};
		info += "Driver: " + std::string((char*)glGetString(GL_VENDOR)) + "\n";
		info += "Renderer: " + std::string((char*)glGetString(GL_RENDERER))+ "\n";
		info += "Version: " + std::string((char*)glGetString(GL_VERSION));
		info += "\nVulkan SDK: " + GetCachedVulkanSdkVersion();
		const VulkanDriverInfo& vkInfo = GetCachedVulkanDriverInfo();
		if (vkInfo.available)
		{
			info += "\nVulkan Driver: " + FormatVulkanDriverVersion(vkInfo.vendorId, vkInfo.driverVersion);
			info += " (" + std::string(VulkanVendorName(vkInfo.vendorId)) + ", " + vkInfo.deviceName + ")";
		}
		else
		{
			info += "\nVulkan Driver: unavailable";
		}
		return info;
	}

}
