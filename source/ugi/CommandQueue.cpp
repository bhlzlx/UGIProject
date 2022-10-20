#include "CommandQueue.h"
#include "CommandBuffer.h"
#include "Semaphore.h"
#include "Device.h"

namespace ugi {

    bool CommandQueue::submitCommandBuffers( const QueueSubmitBatchInfo& batch ) const {
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

        for( uint32_t submitInfoIndex = 0; submitInfoIndex < batch.submitInfoCount; ++submitInfoIndex) {

            for( uint32_t i = 0; i< batch.submitInfos[submitInfoIndex].commandCount; ++i ) {
                commandBuffers[submitInfoIndex][i] = *batch.submitInfos[submitInfoIndex].commandBuffers[i];
            }

            for( uint32_t i = 0; i< batch.submitInfos[submitInfoIndex].semaphoresToWaitCount; ++i ) {
                semaphoresToWait[submitInfoIndex][i] = *batch.submitInfos[submitInfoIndex].semaphoresToWait[i];
                semaphoreToWaitStageFlags[submitInfoIndex][i] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            }

            for( uint32_t i = 0; i< batch.submitInfos[submitInfoIndex].semaphoresToSignalCount; ++i ) {
                semaphoresToSignal[submitInfoIndex][i] = *batch.submitInfos[submitInfoIndex].semaphoresToSignal[i];
            }
            //
            submitInfo[submitInfoIndex].pNext = nullptr; // 
            submitInfo[submitInfoIndex].sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo[submitInfoIndex].signalSemaphoreCount = batch.submitInfos[submitInfoIndex].semaphoresToSignalCount;
			if (submitInfo[submitInfoIndex].signalSemaphoreCount) {
				submitInfo[submitInfoIndex].pSignalSemaphores = semaphoresToSignal[submitInfoIndex];
			}
            submitInfo[submitInfoIndex].waitSemaphoreCount = batch.submitInfos[submitInfoIndex].semaphoresToWaitCount;
			if (submitInfo[submitInfoIndex].waitSemaphoreCount) {
				submitInfo[submitInfoIndex].pWaitSemaphores = semaphoresToWait[submitInfoIndex];
			}
            submitInfo[submitInfoIndex].pCommandBuffers = commandBuffers[submitInfoIndex];
            submitInfo[submitInfoIndex].commandBufferCount = batch.submitInfos[submitInfoIndex].commandCount;
            submitInfo[submitInfoIndex].pWaitDstStageMask = semaphoreToWaitStageFlags[submitInfoIndex]; // 这个以后得改，因为以后不止做渲染队列使用
        }
		VkFence fence = VK_NULL_HANDLE;
		if (batch.fenceToSignal) {
			fence = (VkFence)(*batch.fenceToSignal);
			batch.fenceToSignal->markAsToBeSignal();
		}
        VkResult rst = vkQueueSubmit( _queue, batch.submitInfoCount, submitInfo, fence );
        if( rst == VK_SUCCESS ) {
            return true;
        }
        return false;
    }

    void CommandQueue::waitIdle() const {
        vkQueueWaitIdle( _queue );
    }

    CommandBuffer* CommandQueue::createCommandBuffer( Device* device ) {
        if( VK_NULL_HANDLE == _commandPool) {
            VkCommandPoolCreateInfo commandPoolInfo = {};{
                commandPoolInfo.pNext = nullptr;
                commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
                commandPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
                commandPoolInfo.queueFamilyIndex =  _queueFamilyIndex;
            }
            VkResult rst = vkCreateCommandPool( device->device(), &commandPoolInfo, nullptr, &_commandPool );
            if( VK_SUCCESS!=rst) {
                return nullptr;
            }
        }
        VkCommandBufferAllocateInfo cmdbuffAllocateInfo = {};{
            cmdbuffAllocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            cmdbuffAllocateInfo.pNext = nullptr;
            cmdbuffAllocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            cmdbuffAllocateInfo.commandPool = _commandPool;
            cmdbuffAllocateInfo.commandBufferCount = 1;
        }
        VkCommandBuffer cmdbuff;
        VkResult rst = vkAllocateCommandBuffers( device->device(), &cmdbuffAllocateInfo,&cmdbuff);
        if( VK_SUCCESS != rst ) {
            return nullptr;
        }
        CommandBuffer* cb = new CommandBuffer( device->device(), cmdbuff );
        return cb;
    }

    void CommandQueue::destroyCommandBuffer( Device* device, CommandBuffer* commandBuffer ) {
        VkCommandBuffer cmd = *commandBuffer;
        vkFreeCommandBuffers( device->device(), _commandPool, 1, &cmd);
        delete commandBuffer;
    }

}