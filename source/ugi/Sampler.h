#pragma once
#include "VulkanDeclare.h"
#include "VulkanFunctionDeclare.h"
#include "UGIDeclare.h"
#include "UGITypes.h"
#include <cstdint>
#include "UGIUtility.h"
#include "resourcePool/HashObjectPool.h"
#include "UGITypeMapping.h"
#include "Device.h"

namespace ugi {
    VkSampler CreateSampler( Device* device, const sampler_state_t& samplerState );
}