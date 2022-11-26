#pragma once
#ifdef _WIN32
#include <Windows.h>
#endif
#include <ugi/ugi_declare.h>
#include <ugi/vulkan_declare.h>

#ifdef VK_NO_PROTOTYPES
#else
#endif

#include <vector>

namespace vkhelper
{
	uint32_t getMemoryType(VkPhysicalDevice _device, uint32_t _memTypeBits, VkMemoryPropertyFlags properties);
	VkBool32 isDepthFormat( VkFormat _format );
	VkBool32 isCompressedFormat(VkFormat _format);
	VkBool32 isStencilFormat( VkFormat _format );
	void getImageAcessFlagAndPipelineStage(VkImageLayout _layout, VkAccessFlags& _flags, VkPipelineStageFlags& _stages);
}
