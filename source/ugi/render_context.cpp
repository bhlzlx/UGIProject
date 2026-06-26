#include "render_context.h"
#include <ugi/device.h>
#include <ugi/command_queue.h>
#include <ugi/texture.h>
#include <ugi/command_buffer.h>
#include <ugi/asyncload/gpu_asyncload_manager.h>
#include <ugi/descriptor_set_allocator.h>
#include <ugi/swapchain.h>
#include <ugi/uniform_buffer_allocator.h>
#include <ugi/texture_util.h>

namespace ugi {

    StandardRenderContext::StandardRenderContext()
        : _archive(nullptr)
        , _renderSystem(nullptr)
        , _device(nullptr)
        , _swapchain(nullptr)
        , _frameCompleteFences{}
        , _renderCompleteSemaphores{}
        // , _commandBuffers{}
        , _graphicsQueue(nullptr)
        , _uploadQueue(nullptr)
        , _uniformAllocator(nullptr)
        , _descriptorSetAllocator(nullptr)
        , _asyncLoadManager(nullptr)
        , _flightIndex(0)
        , _imageIndex(0)
    {
    }

    bool StandardRenderContext::initialize(void* wnd, ugi::device_descriptor_t deviceDesc, comm::IArchive* archive) {
        _archive = archive;
        _renderSystem = new RenderSystem();
        _device = _renderSystem->createDevice(deviceDesc, archive);
        _uniformAllocator = _device->createUniformAllocator();
        _descriptorSetAllocator = _device->descriptorSetAllocator();
        _swapchain = _device->createSwapchain(wnd);
        _graphicsQueue = _device->graphicsQueues()[0];
        _uploadQueue = _device->transferQueues()[0];
        _asyncLoadManager = new ugi::GPUAsyncLoadManager();
        for( size_t i = 0; i<MaxFlightCount; ++i) {
            _frameCompleteFences[i] = _device->createFence();
        }
        // 信号量按 swapchain image 数量分配, 用 image index 索引
        // swapchain 最少 MaxFlightCount 张图, 信号量按此分配
        const uint32_t imageCount = 4;
        _renderCompleteSemaphores.resize(imageCount);
        for (uint32_t i = 0; i < imageCount; ++i) {
            _renderCompleteSemaphores[i] = _device->createSemaphore();
        }
        return true;
    }

    bool StandardRenderContext::onPreTick() {
        _device->waitForFence(_frameCompleteFences[_flightIndex] );
        // swapchain 未就绪 (Android 窗口尺寸未定等) → 跳过渲染
        if (!_swapchain || !_swapchain->ready()) return false;
        // status tick
        _device->cycleInvoker().tick();
        _descriptorSetAllocator->tick();
        _uniformAllocator->tick();
        //
        auto cb = _graphicsQueue->createCommandBuffer(_device, CmdbufType::Transient);
        cb->beginEncode(); {
            _asyncLoadManager->tick(cb);
            cb->endEncode();
        }
        _device->cycleInvoker().postCallable([this, cb](){
            _graphicsQueue->destroyCommandBuffer(_device, cb);
        });
        this->submitCommand({{cb}, {}, {}});
        _imageIndex = _swapchain->acquireNextImage(_device, _flightIndex);
        _mainRenderPass = _swapchain->renderPass(_imageIndex);
        return true;
    }

    bool StandardRenderContext::onPostTick() {
#ifndef NDEBUG
        assert(_submits.size() > 1);
        auto const& lastSubmit = _submits.back();
        bool signaledCompleteSemaphore = false;
        for(auto sem: lastSubmit.semaphoresSignals_) {
            if(sem == renderCompleteSemephore()) {
                signaledCompleteSemaphore = true;
                break;
            }
        }
        if(!signaledCompleteSemaphore) {
            return false;
        }
#endif
        std::vector<QueueSubmitInfo> submitInfos;
        for(queue_submit_t& submit: _submits) {
            submitInfos.emplace_back(
                submit.commandBuffers_.data(),
                (uint32_t)submit.commandBuffers_.size(),
                submit.semaphoresWaits_.data(),
                (uint32_t)submit.semaphoresWaits_.size(),
                submit.semaphoresSignals_.data(),
                (uint32_t)submit.semaphoresSignals_.size()
            );
        }
		QueueSubmitBatchInfo submitBatch(submitInfos.data(), submitInfos.size(), _frameCompleteFences[_flightIndex]);
        bool submitRst = _graphicsQueue->submitCommandBuffers(submitBatch);
        _submits.clear();
        if(submitRst) {
            _swapchain->present(_device, _graphicsQueue, _renderCompleteSemaphores[_imageIndex]);
        }
        ++_flightIndex;
        _flightIndex = _flightIndex % MaxFlightCount;
        return submitRst;
    } 

    bool StandardRenderContext::onResize(uint32_t width, uint32_t height) {
        return _swapchain->resize(_device, width, height);
    }

    CommandQueue* StandardRenderContext::primaryQueue() const {
        return _graphicsQueue;
    }

    CommandQueue* StandardRenderContext::transferQueue() const {
        return _uploadQueue;
    }

    GPUAsyncLoadManager* StandardRenderContext::asyncLoadManager() const {
        return _asyncLoadManager;
    }

    UniformAllocator* StandardRenderContext::uniformAllocator() const {
        return _uniformAllocator;
    }

    Semaphore* StandardRenderContext::renderCompleteSemephore() const {
        return _renderCompleteSemaphores[_imageIndex];
    }

    Semaphore* StandardRenderContext::mainFramebufferAvailSemaphore() const {
        return _swapchain->imageAvailSemaphore();
    }

    Device* StandardRenderContext::device() const {
        return _device;
    }

    RenderSystem* StandardRenderContext::renderSystem() const {
        return _renderSystem;
    }

    void StandardRenderContext::submitCommand(queue_submit_t&& submit) {
        _submits.push_back(std::move(submit));
    }

    IRenderPass* StandardRenderContext::mainFramebuffer() const {
        return _mainRenderPass;
    }

    comm::IArchive* StandardRenderContext::archive() const {
        return _archive;
    }

    void StandardRenderContext::updateTexture(
        Texture* texture,
        const image_region_t* regions, uint32_t count, 
        uint8_t const* data, uint32_t dataSize, uint64_t const* offsets,
        AsyncLoadCallback &&callback
    ) {
        texture->updateRegions(_device, regions, count, data, dataSize, offsets, _asyncLoadManager, std::move(callback));
    }

    Texture* StandardRenderContext::createTexture(tex_desc_t const& desc) {
        return _device->createTexture(desc);
    }

    Texture* StandardRenderContext::createTexturePNG(uint8_t const* data, uint32_t length, AsyncLoadCallback&& asyncCallback) {
        Texture* tex = CreateTexturePNG(_device, data, length, _asyncLoadManager, std::move(asyncCallback));
        return tex;
    }

}