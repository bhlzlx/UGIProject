#include "gpu_asyncload_item.h"
#include <device.h>
#include <Semaphore.h>
#include <command_queue.h>
#include <render_components/mesh.h>

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

}