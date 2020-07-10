#include "CommandQueue.h"
#include "CommandBuffer.h"
#include "Semaphore.h"
#include "Device.h"

namespace ugi {

    bool CommandQueue::submitCommandBuffers( const QueueSubmitBatchInfo& _batch ) const {
        //
        constexpr uint32_t MaxSubmitInfoSupport = 8;
        constexpr uint32_t MaxCommandBufferPerSubmit = 8;
        constexpr uint32_t MaxSemWaitPerSubmit = 8;
        constexpr uint32_t MaxSemSignalPerSubmit = 8;
        //

        VkSubmitInfo submitInfo[MaxSubmitInfoSupport] = {};

        VkCommandBuffer commandBuffers[MaxCommandBufferPerSubmit][MaxSubmitInfoSupport];
        VkSemaphore semaphoresToWait[MaxSemWaitPerSubmit][MaxSubmitInfoSupport];
        VkPipelineStageFlags semaphoreToWaitStageFlags[MaxSemWaitPerSubmit][MaxSubmitInfoSupport];
        VkSemaphore semaphoresToSignal[MaxSemSignalPerSubmit][MaxSubmitInfoSupport];

        for( uint32_t submitInfoIndex = 0; submitInfoIndex < _batch.submitInfoCount; ++submitInfoIndex) {

            for( uint32_t i = 0; i< _batch.submitInfos[submitInfoIndex].commandCount; ++i ) {
                commandBuffers[submitInfoIndex][i] = *_batch.submitInfos[submitInfoIndex].commandBuffers[i];
            }

            for( uint32_t i = 0; i< _batch.submitInfos[submitInfoIndex].semaphoresToWaitCount; ++i ) {
                semaphoresToWait[submitInfoIndex][i] = *_batch.submitInfos[submitInfoIndex].semaphoresToWait[i];
                semaphoreToWaitStageFlags[submitInfoIndex][i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }

            for( uint32_t i = 0; i< _batch.submitInfos[submitInfoIndex].semaphoresToSignalCount; ++i ) {
                semaphoresToSignal[submitInfoIndex][i] = *_batch.submitInfos[submitInfoIndex].semaphoresToSignal[i];
            }
            //
            submitInfo[submitInfoIndex].pNext = nullptr; // 
            submitInfo[submitInfoIndex].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo[submitInfoIndex].signalSemaphoreCount = _batch.submitInfos[submitInfoIndex].semaphoresToSignalCount;
            submitInfo[submitInfoIndex].pSignalSemaphores = semaphoresToSignal[submitInfoIndex];
            submitInfo[submitInfoIndex].waitSemaphoreCount = _batch.submitInfos[submitInfoIndex].semaphoresToWaitCount;
            submitInfo[submitInfoIndex].pWaitSemaphores = semaphoresToWait[submitInfoIndex];
            submitInfo[submitInfoIndex].pCommandBuffers = commandBuffers[submitInfoIndex];
            submitInfo[submitInfoIndex].commandBufferCount = _batch.submitInfos[submitInfoIndex].commandCount;
            submitInfo[submitInfoIndex].pWaitDstStageMask = semaphoreToWaitStageFlags[submitInfoIndex]; // 这个以后得改，因为以后不止做渲染队列使用
        }
        VkFence fence = _batch.fenceToSignal? _batch.fenceToSignal->operator VkFence() : VK_NULL_HANDLE;
        VkResult rst = vkQueueSubmit( m_queue, _batch.submitInfoCount, submitInfo, fence );
        if( fence ) {
            _batch.fenceToSignal->waitToBeSignal();
        }
        if( rst == VK_SUCCESS ) {
            return true;
        }
        return false;
    }

    void CommandQueue::waitIdle() const {
        vkQueueWaitIdle( m_queue );
    }

    CommandBuffer* CommandQueue::createCommandBuffer( Device* _device ) {
        if( VK_NULL_HANDLE == m_commandPool) {
            VkCommandPoolCreateInfo commandPoolInfo = {};{
                commandPoolInfo.pNext = nullptr;
                commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                commandPoolInfo.queueFamilyIndex =  m_queueFamilyIndex;
            }
            VkResult rst = vkCreateCommandPool( _device->device(), &commandPoolInfo, nullptr, &m_commandPool );
            if( VK_SUCCESS!=rst) {
                return nullptr;
            }
        }
        VkCommandBufferAllocateInfo cmdbuffAllocateInfo = {};{
            cmdbuffAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdbuffAllocateInfo.pNext = nullptr;
            cmdbuffAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdbuffAllocateInfo.commandPool = m_commandPool;
            cmdbuffAllocateInfo.commandBufferCount = 1;
        }
        VkCommandBuffer cmdbuff;
        VkResult rst = vkAllocateCommandBuffers( _device->device(), &cmdbuffAllocateInfo,&cmdbuff);
        if( VK_SUCCESS != rst ) {
            return nullptr;
        }
        CommandBuffer* cb = new CommandBuffer( _device->device(), cmdbuff );
        return cb;
    }

}