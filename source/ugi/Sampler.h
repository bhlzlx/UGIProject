#pragma once
#include "vulkan_declare.h"
#include "vulkan_function_declare.h"
#include "ugi_declare.h"
#include "ugi_types.h"
#include <cstdint>
#include "ugi_utility.h"
#include "resource_pool/hash_object_pool.h"
#include "ugi_type_mapping.h"
#include "device.h"

namespace ugi {
    VkSampler CreateSampler( Device* device, const sampler_state_t& samplerState );
}