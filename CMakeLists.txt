cmake_minimum_required(VERSION 3.20)

project(Syndra-Engine LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
endif()

if(CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release;Dist" CACHE STRING "" FORCE)
else()
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "" FORCE)
  endif()
endif()

set(CMAKE_C_FLAGS_DIST "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_DIST "${CMAKE_CXX_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DIST "${CMAKE_EXE_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DIST "${CMAKE_SHARED_LINKER_FLAGS_RELEASE}" CACHE STRING "" FORCE)

set(SYNDRA_ROOT_DIR "${CMAKE_SOURCE_DIR}")
set(SYNDRA_OUTPUT_PLATFORM "windows-x86_64")

set(SYNDRA_VULKAN_SDK "$ENV{VULKAN_SDK}")
if(NOT SYNDRA_VULKAN_SDK)
  message(FATAL_ERROR "VULKAN_SDK environment variable is not set.")
endif()

set(SYNDRA_VULKAN_INCLUDE_DIR "${SYNDRA_VULKAN_SDK}/Include")
set(SYNDRA_VULKAN_LIB_DIR "${SYNDRA_VULKAN_SDK}/Lib")
set(SYNDRA_VULKAN_BIN_DIR "${SYNDRA_VULKAN_SDK}/Bin")

if(NOT EXISTS "${SYNDRA_VULKAN_INCLUDE_DIR}")
  message(FATAL_ERROR "Vulkan SDK include directory not found: ${SYNDRA_VULKAN_INCLUDE_DIR}")
endif()
if(NOT EXISTS "${SYNDRA_VULKAN_LIB_DIR}")
  message(FATAL_ERROR "Vulkan SDK lib directory not found: ${SYNDRA_VULKAN_LIB_DIR}")
endif()
if(NOT EXISTS "${SYNDRA_VULKAN_BIN_DIR}")
  message(FATAL_ERROR "Vulkan SDK bin directory not found: ${SYNDRA_VULKAN_BIN_DIR}")
endif()

if(CMAKE_CONFIGURATION_TYPES)
  set(SYNDRA_CONFIGS ${CMAKE_CONFIGURATION_TYPES})
else()
  set(SYNDRA_CONFIGS ${CMAKE_BUILD_TYPE})
endif()

function(syndra_set_output_dirs target_name)
  foreach(cfg IN LISTS SYNDRA_CONFIGS)
    string(TOLOWER "${cfg}" cfg_lower)
    string(TOUPPER "${cfg}" cfg_upper)
    set(output_dir "${SYNDRA_ROOT_DIR}/bin/${cfg_lower}-${SYNDRA_OUTPUT_PLATFORM}/${target_name}")
    set_target_properties(${target_name} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY_${cfg_upper} "${output_dir}"
      LIBRARY_OUTPUT_DIRECTORY_${cfg_upper} "${output_dir}"
      ARCHIVE_OUTPUT_DIRECTORY_${cfg_upper} "${output_dir}"
    )
  endforeach()
endfunction()

function(syndra_add_config_defines target_name)
  target_compile_definitions(${target_name} PRIVATE
    $<$<CONFIG:Debug>:SN_DEBUG>
    $<$<CONFIG:Release>:SN_RELEASE>
    $<$<CONFIG:Dist>:SN_DIST>
  )
endfunction()

function(syndra_add_editor_runtime_deps target_name)
  if(NOT WIN32)
    return()
  endif()

  set(assimp_debug "${SYNDRA_ROOT_DIR}/Syndra/vendor/assimp/build/Debug/assimp-vc142-mtd.dll")
  set(assimp_release "${SYNDRA_ROOT_DIR}/Syndra/vendor/assimp/build/Release/assimp-vc142-mt.dll")

  if(NOT EXISTS "${assimp_debug}")
    message(WARNING "Assimp debug DLL not found: ${assimp_debug}")
  endif()
  if(NOT EXISTS "${assimp_release}")
    message(WARNING "Assimp release DLL not found: ${assimp_release}")
  endif()

  add_custom_command(TARGET ${target_name} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "$<IF:$<CONFIG:Debug>,${assimp_debug},${assimp_release}>"
            "$<TARGET_FILE_DIR:${target_name}>"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SYNDRA_VULKAN_BIN_DIR}"
            "$<TARGET_FILE_DIR:${target_name}>"
    VERBATIM
  )
endfunction()

function(syndra_set_debug_working_dir target_name working_dir)
  if(MSVC)
    set_property(TARGET ${target_name} PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${working_dir}")
  endif()
endfunction()

function(syndra_disable_vcpkg_applocal target_name)
  if(MSVC)
    set_property(TARGET ${target_name} PROPERTY VS_GLOBAL_VcpkgApplocalDeps false)
  endif()
endfunction()

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)

set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(YAML_CPP_INSTALL OFF CACHE BOOL "" FORCE)
set(YAML_BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

include(FetchContent)

option(SYNDRA_USE_VULKAN_SDK_SPIRV_CROSS
  "Link SPIRV-Cross binaries from Vulkan SDK instead of building SPIRV-Cross from source."
  OFF
)

if(NOT SYNDRA_USE_VULKAN_SDK_SPIRV_CROSS)
  set(SPIRV_CROSS_ENABLE_TESTS OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_CPP OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_C_API OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_REFLECT OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_UTIL OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_HLSL OFF CACHE BOOL "" FORCE)
  set(SPIRV_CROSS_ENABLE_MSL OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    spirv_cross
    GIT_REPOSITORY https://github.com/KhronosGroup/SPIRV-Cross.git
    GIT_TAG vulkan-sdk-1.4.335.0
    GIT_SHALLOW TRUE
  )
  FetchContent_MakeAvailable(spirv_cross)
endif()

FetchContent_Declare(
  volk
  GIT_REPOSITORY https://github.com/zeux/volk.git
  GIT_TAG vulkan-sdk-1.4.335.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(volk)

FetchContent_Declare(
  vulkan_memory_allocator
  GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
  GIT_TAG v3.3.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(vulkan_memory_allocator)

add_library(VulkanMemoryAllocatorHeaders INTERFACE)
target_include_directories(VulkanMemoryAllocatorHeaders INTERFACE
  ${vulkan_memory_allocator_SOURCE_DIR}/include
)

add_subdirectory(Syndra/vendor/GLFW)
add_subdirectory(Syndra/vendor/yaml-cpp)
add_subdirectory(Syndra/vendor/Glad)

set(SYNDRA_IMGUI_DIR "${SYNDRA_ROOT_DIR}/Syndra/vendor/imgui")
add_library(imgui STATIC
  ${SYNDRA_IMGUI_DIR}/imgui.cpp
  ${SYNDRA_IMGUI_DIR}/imgui_demo.cpp
  ${SYNDRA_IMGUI_DIR}/imgui_draw.cpp
  ${SYNDRA_IMGUI_DIR}/imgui_tables.cpp
  ${SYNDRA_IMGUI_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui PUBLIC
  ${SYNDRA_IMGUI_DIR}
)
target_compile_features(imgui PUBLIC cxx_std_17)

set(SYNDRA_IMGUIZMO_DIR "${SYNDRA_ROOT_DIR}/Syndra/vendor/ImGuizmo")
add_library(ImGuizmo STATIC
  ${SYNDRA_IMGUIZMO_DIR}/ImCurveEdit.cpp
  ${SYNDRA_IMGUIZMO_DIR}/ImGradient.cpp
  ${SYNDRA_IMGUIZMO_DIR}/ImGuizmo.cpp
  ${SYNDRA_IMGUIZMO_DIR}/ImSequencer.cpp
)
target_include_directories(ImGuizmo PUBLIC
  ${SYNDRA_IMGUIZMO_DIR}
)
target_link_libraries(ImGuizmo PUBLIC imgui)
target_compile_features(ImGuizmo PUBLIC cxx_std_17)

add_subdirectory(Syndra)
add_subdirectory(Syndra-Editor)
add_subdirectory(Sandbox)

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT "Syndra-Editor")
