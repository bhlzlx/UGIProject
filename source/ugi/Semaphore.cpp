#include "Semaphore.h"

namespace ugi {

    bool Fence::IsSignaled( VkDevice device, const Fence& fence ) {
        return (VK_SUCCESS == vkGetFenceStatus( device, fence._fence));
    }

}