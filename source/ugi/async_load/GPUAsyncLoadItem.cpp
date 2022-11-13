#include "GPUAsyncLoadItem.h"
#include <Device.h>
#include <Semaphore.h>
#include <CommandQueue.h>
#include <render_components/MeshPrimitive.h>

namespace ugi {

    bool GPUAsyncLoadItem::isComplete() {
        if(fence_) {
            auto rst = device_->isSignaled(fence_);
            if(rst) {
                device_->destroyFence(fence_);
                fence_ = nullptr;
            }
            return rst;
        }
        return true;
    }

    void GPUAsyncLoadItem::onLoadComplete(CommandBuffer* cb) {
        if(fence_) {
            device_->destroyFence(fence_);
        }
        onComplete_(cb);
    }

    // void GPUMeshAsyncLoadItem::onLoadComplete(CommandBuffer* cb) {
    //     device_->destroyBuffer(stagingBuffer_);
    //     mesh_->uploaded_ = true;
    //     queue_->destroyCommandBuffer(device_, cb_);
    // }

    // void GPUBufferToImageItem::onLoadComplete(CommandBuffer* cb) {
    //     device_->destroyBuffer(stagingBuffer_);
    //     mesh_->uploaded_ = true;
    //     queue_->destroyCommandBuffer(device_, cb_);
    // }

}