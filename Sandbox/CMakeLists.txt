set(SANDBOX_SOURCES
  src/SandboxApp.cpp
)

set(SYNDRA_VULKAN_BOOTSTRAP_SOURCES
  src/VulkanBootstrap.cpp
)

add_executable(Sandbox
  ${SANDBOX_SOURCES}
)

target_include_directories(Sandbox PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SYNDRA_ROOT_DIR}/Syndra/vendor/imgui
)

target_link_libraries(Sandbox PRIVATE
  Syndra
)

syndra_set_output_dirs(Sandbox)
syndra_add_config_defines(Sandbox)
syndra_set_debug_working_dir(Sandbox "${SYNDRA_ROOT_DIR}/Syndra-Editor")
syndra_disable_vcpkg_applocal(Sandbox)

add_executable(Syndra-VulkanBootstrap
  ${SYNDRA_VULKAN_BOOTSTRAP_SOURCES}
)

target_include_directories(Syndra-VulkanBootstrap PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Syndra-VulkanBootstrap PRIVATE
  Syndra
)

syndra_set_output_dirs(Syndra-VulkanBootstrap)
syndra_add_config_defines(Syndra-VulkanBootstrap)
syndra_add_editor_runtime_deps(Syndra-VulkanBootstrap)
syndra_set_debug_working_dir(Syndra-VulkanBootstrap "${SYNDRA_ROOT_DIR}/Syndra-Editor")
syndra_disable_vcpkg_applocal(Syndra-VulkanBootstrap)
