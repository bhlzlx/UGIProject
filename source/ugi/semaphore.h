#pragma once

#include "vulkan_declare.h"
#include "vulkan_function_declare.h"
#include <vector>

namespace ugi {

    /**
     * @brief light-weight VkSemaphore wrapper \n
     * light-weight VkSemaphore
     * 
     * 因为 `semaphore` 的状态是 `GPU` 的内部状态，所以我们在 `CPU` 端是没办法知道它的确切状态的
     * 
     */
    class Semaphore {
        friend class Device;
    private:
        VkSemaphore _semaphore;
        //
        Semaphore( VkSemaphore sem = 0 ) 
            : _semaphore( sem )
        {
        }
        Semaphore( const Semaphore& sem ) {
            _semaphore = sem._semaphore;
        }
    public:
        operator VkSemaphore () {
            return _semaphore;
        }
        operator VkSemaphore*() {
            return &_semaphore;
        }
        void release(Device* device);
    };

    class Fence {
        friend class Device;
    private:
        VkFence _fence;
        bool m_waitToSignaled;
        //
        Fence( VkFence _fence )
            : _fence( _fence )
            , m_waitToSignaled( false )
        {
        }
    public:
        static bool IsSignaled( VkDevice device, const Fence& fence );
        operator VkFence() {
            return _fence;
        }
        void markAsToBeSignal() {
            m_waitToSignaled = true;
        }
        void release(Device* device);
    };

}