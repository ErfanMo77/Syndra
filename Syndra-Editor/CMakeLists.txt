set(SYNDRA_EDITOR_SOURCES
  src/EditorLayer.cpp
  src/SyndraEditorApp.cpp
  src/UI/Panels/CameraPanel.cpp
  src/UI/Panels/ContentBrowser.cpp
  src/UI/Panels/LightPanel.cpp
  src/UI/Panels/MaterialPanel.cpp
  src/UI/Panels/MeshPanel.cpp
  src/UI/ScenePanel.cpp
)

set(SYNDRA_EDITOR_HEADERS
  src/EditorLayer.h
  src/UI/Panels/CameraPanel.h
  src/UI/Panels/ContentBrowser.h
  src/UI/Panels/LightPanel.h
  src/UI/Panels/MaterialPanel.h
  src/UI/Panels/MeshPanel.h
  src/UI/ScenePanel.h
  src/UI/UI.h
)

add_executable(Syndra-Editor
  ${SYNDRA_EDITOR_SOURCES}
  ${SYNDRA_EDITOR_HEADERS}
)

target_include_directories(Syndra-Editor PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${SYNDRA_ROOT_DIR}/Syndra/vendor
  ${SYNDRA_ROOT_DIR}/Syndra/vendor/imgui
  ${SYNDRA_ROOT_DIR}/Syndra/vendor/ImGuizmo
)

target_link_libraries(Syndra-Editor PRIVATE
  Syndra
)

syndra_set_output_dirs(Syndra-Editor)
syndra_add_config_defines(Syndra-Editor)
syndra_add_editor_runtime_deps(Syndra-Editor)
syndra_set_debug_working_dir(Syndra-Editor "${SYNDRA_ROOT_DIR}/Syndra-Editor")
