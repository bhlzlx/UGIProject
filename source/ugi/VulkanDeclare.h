#pragma once
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#ifdef _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
#endif