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
        VkSemaphore m_semaphore;
        //
        Semaphore( VkSemaphore _sem = 0 ) 
            : m_semaphore( _sem )
        {
        }
        Semaphore( const Semaphore& _sem ) {
            m_semaphore = _sem.m_semaphore;
        }
    public:
        operator VkSemaphore () {
            return m_semaphore;
        }
        operator VkSemaphore*() {
            return &m_semaphore;
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