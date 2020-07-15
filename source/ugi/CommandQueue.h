#pragma once

#include "UGIDeclare.h"
#include "UGITypes.h"
#include "VulkanDeclare.h"
#include "ResourceManager.h"
#include <cstdint>

namespace ugi {
    struct QueueSubmitInfo {
        //
        CommandBuffer** commandBuffers = nullptr;
        uint32_t        commandCount = 0;
        //
        Semaphore**     semaphoresToWait = nullptr;
        uint32_t        semaphoresToWaitCount = 0;
        Semaphore**     semaphoresToSignal = nullptr;
        uint32_t        semaphoresToSignalCount = 0;
    };

    struct QueueSubmitBatchInfo {
        QueueSubmitInfo*    submitInfos = nullptr;
        uint32_t            submitInfoCount = 0;
        //
        Fence*              fenceToSignal = nullptr;
    };


    class CommandQueue {
    private:
        VkQueue         m_queue;
        uint32_t        m_queueFamilyIndex;
        uint32_t        m_queueIndex;
        //
        VkCommandPool   m_commandPool;
        //
    public:
        /* ============================================
            method : submitCommandBuffers
        / ============================================*/
        bool submitCommandBuffers( const QueueSubmitBatchInfo& _batch ) const;
        void waitIdle() const;
        //
        CommandQueue( VkQueue _queue, uint32_t _queueFamilyIndex, uint32_t _queueIndex ) 
            : m_queue( _queue )
            , m_queueFamilyIndex( _queueFamilyIndex )
            , m_queueIndex( _queueIndex )
            , m_commandPool(VK_NULL_HANDLE)
        {
        }
        //
        CommandBuffer* createCommandBuffer( Device* _device );
        void destroyCommandBuffer( Device* device, CommandBuffer* commandBuffer );
        //
        operator VkQueue() const {
            return m_queue;
        }
    };

}