#include "Semaphore.h"

namespace ugi {

    bool Fence::IsSignaled( VkDevice _device, const Fence& _fence ) {
        return (VK_SUCCESS == vkGetFenceStatus( _device, _fence.m_fence));
    }

}