#include "semaphore.h"
#include <device.h>

namespace ugi {

    void Semaphore::release(Device* device) {
        vkDestroySemaphore(device->device(), _semaphore, nullptr);
        delete this;
    }

    void Fence::release(Device* device) {
        vkDestroyFence(device->device(), _fence, nullptr);
    }

    bool Fence::IsSignaled( VkDevice device, const Fence& fence ) {
        return (VK_SUCCESS == vkGetFenceStatus( device, fence._fence));
    }

}