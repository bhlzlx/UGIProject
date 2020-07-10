#define VK_NO_PROTOTYPES
#include <vulkan/vulkan_core.h>
#undef REGIST_VULKAN_FUNCTION
#define REGIST_VULKAN_FUNCTION(FUNCTION) PFN_##FUNCTION FUNCTION;
#include "VulkanFunctionRegistList.h"