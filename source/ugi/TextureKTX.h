#pragma once

#include <ugi/UGIDeclare.h>
#include <cstdint>
#include <functional>

namespace ugi {
    
    Texture* CreateTextureKTX(Device* device, uint8_t const* data, uint32_t dataLen, GPUAsyncLoadManager* asyncLoadMgr, std::function<void(void* res, CommandBuffer*)>&& callback);

}