#pragma once
#define VMA_STATIC_VULKAN_FUNCTIONS 1
#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
// declare vulkan api - core
// Android: VMA 用 extern "C" 声明, 这里必须一致
#ifdef __ANDROID__
extern "C" {
#endif
#undef REGIST_VULKAN_FUNCTION
#define REGIST_VULKAN_FUNCTION( FUNCTION ) extern PFN_##FUNCTION FUNCTION;
#include "vulkan_function_regist_list.h"
#ifdef __ANDROID__
}
#endif