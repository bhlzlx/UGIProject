#pragma once

#include "VulkanDeclare.h"
#include "VulkanFunctionDeclare.h"
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
        Semaphore( VkSemaphore _sem = 0 ) 
            : _semaphore( _sem )
        {
        }
        Semaphore( const Semaphore& _sem ) {
            _semaphore = _sem._semaphore;
        }
    public:
        operator VkSemaphore () {
            return _semaphore;
        }
        operator VkSemaphore*() {
            return &_semaphore;
        }
    };

    class Fence {
        friend class Device;
    private:
        VkFence m_fence;
        bool m_waitToSignaled;
        //
        Fence( VkFence _fence )
            : m_fence( _fence )
            , m_waitToSignaled( false )
        {
        }
    public:
        static bool IsSignaled( VkDevice _device, const Fence& _fence );
        operator VkFence() {
            return m_fence;
        }
        void waitToBeSignal() {
            m_waitToSignaled = true;
        }
    };

}