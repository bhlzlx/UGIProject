#pragma once

#include "ugi_declare.h"
#include "ugi_types.h"
#include "vulkan_declare.h"
#include "flight_cycle_invoker.h"
#include <cstdint>

namespace ugi {

    struct QueueSubmitInfo {
        CommandBuffer** commandBuffers = nullptr;
        uint32_t        commandCount = 0;
        //
        Semaphore**     semaphoresToWait = nullptr;
        uint32_t        semaphoresToWaitCount = 0;
        Semaphore**     semaphoresToSignal = nullptr;
        uint32_t        semaphoresToSignalCount = 0;
        //
		QueueSubmitInfo(CommandBuffer** pCmdBuffs, uint32_t cmdCount, Semaphore** pWaitSems, uint32_t semWaitCount, Semaphore** pSigSems, uint32_t semSigCount)
			: commandBuffers(pCmdBuffs)
			, commandCount(cmdCount)
			, semaphoresToWait(pWaitSems)
			, semaphoresToWaitCount(semWaitCount)
			, semaphoresToSignal(pSigSems)
			, semaphoresToSignalCount(semSigCount) {
		}
    };

    struct QueueSubmitBatchInfo {
        QueueSubmitInfo*    submitInfos = nullptr;
        uint32_t            submitInfoCount = 0;
        Fence*              fenceToSignal = nullptr;
		QueueSubmitBatchInfo(QueueSubmitInfo* pSubmitInfos, uint32_t infoCount, Fence* fenceSig)
			: submitInfos(pSubmitInfos)
			, submitInfoCount(infoCount)
			, fenceToSignal(fenceSig)
		{
		}
	};


    class CommandQueue {
    private:
        VkQueue         _queue;
        uint32_t        _queueFamilyIndex;
        uint32_t        _queueIndex;
        //
        VkCommandPool   _resetablePool;
        VkCommandPool   _transientPool;
        //
    public:
        /* ============================================
            method : submitCommandBuffers
        / ============================================*/
        bool submitCommandBuffers( const QueueSubmitBatchInfo& batch ) const;
        void waitIdle() const;
        //
        CommandQueue( VkQueue queue, uint32_t queueFamilyIndex, uint32_t queueIndex ) 
            : _queue( queue )
            , _queueFamilyIndex( queueFamilyIndex )
            , _queueIndex( queueIndex )
            , _resetablePool(VK_NULL_HANDLE)
            , _transientPool(VK_NULL_HANDLE)
        {
        }
        //
        CommandBuffer* createCommandBuffer(Device* device, CmdbufType type = CmdbufType::Resetable);
        void destroyCommandBuffer( Device* device, CommandBuffer* commandBuffer );
        //
        operator VkQueue() const {
            return _queue;
        }
    };

}