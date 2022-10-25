#include "GPUAsyncLoadItem.h"
#include <Device.h>
#include <Semaphore.h>
#include <render_components/MeshPrimitive.h>

namespace ugi {

    bool IGPUAsyncLoadItem::isComplete() {
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

    void GPUMeshAsyncLoadItem::onLoadComplete() {
        for(auto stagingBuffer : stagingBuffers_) {
            device_->destroyBuffer(stagingBuffer);
        }
        stagingBuffers_.clear();
        mesh_->prepared_ = true;
    }

}