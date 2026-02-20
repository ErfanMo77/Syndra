#include "lpch.h"

#define IMGUI_IMPL_OPENGL_LOADER_GLAD
#define IMGUI_IMPL_VULKAN_USE_VOLK
#include <volk.h>
#ifndef VULKAN_H_
#define VULKAN_H_
#endif

#include "backends/imgui_impl_opengl3.cpp"
#include "backends/imgui_impl_vulkan.cpp"
#include "backends/imgui_impl_glfw.cpp"
