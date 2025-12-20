set(SYNDRA_SOURCES
  src/Engine/Core/Application.cpp
  src/Engine/Core/Layer.cpp
  src/Engine/Core/LayerStack.cpp
  src/Engine/Core/Log.cpp
  src/Engine/Core/Window.cpp
  src/Engine/ImGui/ImGuiBuild.cpp
  src/Engine/ImGui/ImGuiLayer.cpp
  src/Engine/Renderer/Buffer.cpp
  src/Engine/Renderer/DeferredRenderer.cpp
  src/Engine/Renderer/Environment.cpp
  src/Engine/Renderer/ForwardPlusRenderer.cpp
  src/Engine/Renderer/FrameBuffer.cpp
  src/Engine/Renderer/LightManager.cpp
  src/Engine/Renderer/Material.cpp
  src/Engine/Renderer/Mesh.cpp
  src/Engine/Renderer/Model.cpp
  src/Engine/Renderer/OrthographicCamera.cpp
  src/Engine/Renderer/PerspectiveCamera.cpp
  src/Engine/Renderer/RenderCommand.cpp
  src/Engine/Renderer/Renderer.cpp
  src/Engine/Renderer/RendererAPI.cpp
  src/Engine/Renderer/RenderPass.cpp
  src/Engine/Renderer/SceneRenderer.cpp
  src/Engine/Renderer/Shader.cpp
  src/Engine/Renderer/Texture.cpp
  src/Engine/Renderer/UniformBuffer.cpp
  src/Engine/Renderer/VertexArray.cpp
  src/Engine/Scene/Components.cpp
  src/Engine/Scene/Entity.cpp
  src/Engine/Scene/Light.cpp
  src/Engine/Scene/Scene.cpp
  src/Engine/Scene/SceneCamera.cpp
  src/Engine/Scene/SceneSerializer.cpp
  src/Engine/Utils/Math.cpp
  src/lpch.cpp
  src/Platform/OpenGL/OpenGLBuffer.cpp
  src/Platform/OpenGL/OpenGLContext.cpp
  src/Platform/OpenGL/OpenGLFrameBuffer.cpp
  src/Platform/OpenGL/OpenGLRendererAPI.cpp
  src/Platform/OpenGL/OpenGLShader.cpp
  src/Platform/OpenGL/OpenGLTexture1D.cpp
  src/Platform/OpenGL/OpenGLTexture2D.cpp
  src/Platform/OpenGL/OpenGLUniformBuffer.cpp
  src/Platform/OpenGL/OpenGLVertexArray.cpp
  src/Platform/Windows/WindowsInput.cpp
  src/Platform/Windows/WindowsUtils.cpp
  src/Platform/Windows/WindowsWindow.cpp
  vendor/stb_image/stb_image.cpp
)

set(SYNDRA_HEADERS
  src/Engine.h
  src/Engine/Core/Application.h
  src/Engine/Core/Core.h
  src/Engine/Core/EntryPoint.h
  src/Engine/Core/Input.h
  src/Engine/Core/Instrument.h
  src/Engine/Core/KeyCodes.h
  src/Engine/Core/Layer.h
  src/Engine/Core/LayerStack.h
  src/Engine/Core/Log.h
  src/Engine/Core/MouseCodes.h
  src/Engine/Core/Timestep.h
  src/Engine/Core/Window.h
  src/Engine/Events/ApplicationEvent.h
  src/Engine/Events/Event.h
  src/Engine/Events/KeyEvent.h
  src/Engine/Events/MouseEvent.h
  src/Engine/ImGui/IconsFontAwesome5.h
  src/Engine/ImGui/ImGuiLayer.h
  src/Engine/Renderer/Buffer.h
  src/Engine/Renderer/Camera.h
  src/Engine/Renderer/DeferredRenderer.h
  src/Engine/Renderer/Environment.h
  src/Engine/Renderer/ForwardPlusRenderer.h
  src/Engine/Renderer/FrameBuffer.h
  src/Engine/Renderer/GraphicsContext.h
  src/Engine/Renderer/LightManager.h
  src/Engine/Renderer/Material.h
  src/Engine/Renderer/Mesh.h
  src/Engine/Renderer/Model.h
  src/Engine/Renderer/OrthographicCamera.h
  src/Engine/Renderer/PerspectiveCamera.h
  src/Engine/Renderer/RenderCommand.h
  src/Engine/Renderer/Renderer.h
  src/Engine/Renderer/RendererAPI.h
  src/Engine/Renderer/RenderPass.h
  src/Engine/Renderer/RenderPipeline.h
  src/Engine/Renderer/SceneRenderer.h
  src/Engine/Renderer/Shader.h
  src/Engine/Renderer/Texture.h
  src/Engine/Renderer/UniformBuffer.h
  src/Engine/Renderer/VertexArray.h
  src/Engine/Scene/Components.h
  src/Engine/Scene/Entity.h
  src/Engine/Scene/Light.h
  src/Engine/Scene/Scene.h
  src/Engine/Scene/SceneCamera.h
  src/Engine/Scene/SceneSerializer.h
  src/Engine/Utils/Math.h
  src/Engine/Utils/PlatformUtils.h
  src/Engine/Utils/PoissonGenerator.h
  src/lpch.h
  src/Platform/OpenGL/OpenGLBuffer.h
  src/Platform/OpenGL/OpenGLContext.h
  src/Platform/OpenGL/OpenGLFrameBuffer.h
  src/Platform/OpenGL/OpenGLRendererAPI.h
  src/Platform/OpenGL/OpenGLShader.h
  src/Platform/OpenGL/OpenGLTexture1D.h
  src/Platform/OpenGL/OpenGLTexture2D.h
  src/Platform/OpenGL/OpenGLUniformBuffer.h
  src/Platform/OpenGL/OpenGLVertexArray.h
  src/Platform/Windows/WindowsWindow.h
  vendor/stb_image/stb_image.h
)

add_library(Syndra STATIC
  ${SYNDRA_SOURCES}
  ${SYNDRA_HEADERS}
)

target_include_directories(Syndra PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/glm
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/stb_image
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/Entt/Include
  ${CMAKE_CURRENT_SOURCE_DIR}/vendor/assimp/include
  ${SYNDRA_VULKAN_INCLUDE_DIR}
)

target_precompile_headers(Syndra PRIVATE
  src/lpch.h
)

syndra_set_output_dirs(Syndra)
syndra_add_config_defines(Syndra)

if(WIN32)
  target_compile_definitions(Syndra PRIVATE GLFW_INCLUDE_NONE)
endif()

set(SHADERC_DEBUG_LIB "${SYNDRA_VULKAN_LIB_DIR}/shaderc_sharedd.lib")
set(SHADERC_RELEASE_LIB "${SYNDRA_VULKAN_LIB_DIR}/shaderc_shared.lib")
set(SPIRV_CROSS_DEBUG_LIB "${SYNDRA_VULKAN_LIB_DIR}/spirv-cross-cored.lib")
set(SPIRV_CROSS_RELEASE_LIB "${SYNDRA_VULKAN_LIB_DIR}/spirv-cross-core.lib")
set(SPIRV_CROSS_GLSL_DEBUG_LIB "${SYNDRA_VULKAN_LIB_DIR}/spirv-cross-glsld.lib")
set(SPIRV_CROSS_GLSL_RELEASE_LIB "${SYNDRA_VULKAN_LIB_DIR}/spirv-cross-glsl.lib")
set(VULKAN_LIB "${SYNDRA_VULKAN_LIB_DIR}/vulkan-1.lib")

if(NOT EXISTS "${SHADERC_DEBUG_LIB}")
  message(WARNING "shaderc debug library not found: ${SHADERC_DEBUG_LIB}")
endif()
if(NOT EXISTS "${SPIRV_CROSS_DEBUG_LIB}")
  message(WARNING "spirv-cross debug library not found: ${SPIRV_CROSS_DEBUG_LIB}")
endif()
if(NOT EXISTS "${SPIRV_CROSS_GLSL_DEBUG_LIB}")
  message(WARNING "spirv-cross-glsl debug library not found: ${SPIRV_CROSS_GLSL_DEBUG_LIB}")
endif()
if(NOT EXISTS "${VULKAN_LIB}")
  message(WARNING "Vulkan loader library not found: ${VULKAN_LIB}")
endif()

set(ASSIMP_DEBUG_LIB "${CMAKE_CURRENT_SOURCE_DIR}/vendor/assimp/build/Debug/assimp-vc142-mtd.lib")
set(ASSIMP_RELEASE_LIB "${CMAKE_CURRENT_SOURCE_DIR}/vendor/assimp/build/Release/assimp-vc142-mt.lib")

if(NOT EXISTS "${ASSIMP_DEBUG_LIB}")
  message(WARNING "Assimp debug library not found: ${ASSIMP_DEBUG_LIB}")
endif()
if(NOT EXISTS "${ASSIMP_RELEASE_LIB}")
  message(WARNING "Assimp release library not found: ${ASSIMP_RELEASE_LIB}")
endif()

target_link_libraries(Syndra PUBLIC
  glfw
  glad
  imgui
  ImGuizmo
  yaml-cpp
  "${VULKAN_LIB}"
  debug "${SHADERC_DEBUG_LIB}"
  optimized "${SHADERC_RELEASE_LIB}"
  debug "${SPIRV_CROSS_DEBUG_LIB}"
  optimized "${SPIRV_CROSS_RELEASE_LIB}"
  debug "${SPIRV_CROSS_GLSL_DEBUG_LIB}"
  optimized "${SPIRV_CROSS_GLSL_RELEASE_LIB}"
  debug "${ASSIMP_DEBUG_LIB}"
  optimized "${ASSIMP_RELEASE_LIB}"
)

if(WIN32)
  target_link_libraries(Syndra PUBLIC opengl32)
endif()
