#pragma once
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>
#include <ugi/CommandQueue.h>
#include <vector>
#include <LightWeightCommon/lightweight_comm.h>

namespace ugi {

    /**
     * @brief 
     * 一般设备都能保证至少有一个graphics queue和一个transfer queue
     */

    struct queue_submit_t {
        std::vector<CommandBuffer*>     commandBuffers_;
        std::vector<Semaphore*>         semaphoresWaits_;
        std::vector<Semaphore*>         semaphoresSignals_;
    };

    class StandardRenderContext {
    protected:
        comm::IArchive*                 _archive;
        ugi::RenderSystem*              _renderSystem;                                     //
        ugi::Device*                    _device;                                           //
        ugi::Swapchain*                 _swapchain;                                        //
        ugi::Fence*                     _frameCompleteFences[MaxFlightCount];              // command buffer 被GPU消化完会给 fence 一个 signal, 用于双缓冲或者多缓冲逻辑隔帧等待
        ugi::Semaphore*                 _renderCompleteSemaphores[MaxFlightCount];         // 用于GPU内部等待
        // ugi::CommandBuffer*             _commandBuffers[MaxFlightCount];                   // command buffer for each frame
        ugi::CommandQueue*              _graphicsQueue;                                    // graphics queue
        ugi::CommandQueue*              _uploadQueue;                                      // upload queue
        ugi::UniformAllocator*          _uniformAllocator;
        ugi::DescriptorSetAllocator*    _descriptorSetAllocator;
        ugi::GPUAsyncLoadManager*       _asyncLoadManager;
        ugi::IRenderPass*               _mainRenderPass;
        uint32_t                        _flightIndex;
        uint32_t                        _renderPassIndex;
        //
        // std::vector<QueueSubmitInfo>    _submitInfos;
        std::vector<queue_submit_t>     _submits;
    public:
        StandardRenderContext();
        bool initialize(void* _wnd, ugi::DeviceDescriptor deviceDesc, comm::IArchive* archive);
        bool onPreTick(); // sync gpu result
        bool onPostTick(); // present the swapchain
        bool onResize(uint32_t width, uint32_t height);
        //
        CommandQueue* primaryQueue() const;
        CommandQueue* transferQueue() const;
        GPUAsyncLoadManager* asyncLoadManager() const;
        UniformAllocator* uniformAllocator() const;
        Semaphore* renderCompleteSemephore() const;
        Semaphore* mainFramebufferAvailSemaphore() const;
        Device* device() const;
        RenderSystem* renderSystem() const;
        IRenderPass* mainFramebuffer() const;
        comm::IArchive* archive() const;
        //
        void submitCommand(queue_submit_t&& submit);
    };

}