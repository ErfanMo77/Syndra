set(SANDBOX_SOURCES
  src/SandboxApp.cpp
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
