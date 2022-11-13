#pragma once
#include <UGITypes.h>
#include <vector>
#include <functional>

namespace ugi {

    class Mesh;

    class GPUAsyncLoadItem {
    protected:
        Device* device_;
        Fence* fence_;
        std::function<void(CommandBuffer*)> onComplete_;
    public:
        GPUAsyncLoadItem(Device* device, Fence* fence, std::function<void(CommandBuffer*)>&& onComplete) 
            : device_(device)
            , fence_(fence)
            , onComplete_(std::move(onComplete))
        {}
        bool isComplete();
        void onLoadComplete(CommandBuffer* cb);
    };

    // class GPUMeshAsyncLoadItem : public IGPUAsyncLoadItem {
    // private:
    //     Mesh* mesh_;
    //     Buffer* stagingBuffer_;
    //     CommandBuffer* cb_;
    //     CommandQueue* queue_;
    // public:
    //     GPUMeshAsyncLoadItem(Device* device, Fence* fence, Mesh* mesh, Buffer* stagingBuffer, CommandBuffer* cb, CommandQueue* queue, std::function<void()>& onComplete)
    //         : IGPUAsyncLoadItem(device, fence, )
    //         , mesh_(mesh)
    //         , stagingBuffer_(stagingBuffer)
    //         , cb_(cb)
    //         , queue_(queue)
    //     {}
    //     virtual void onLoadComplete(CommandBuffer* cb) override;
    // };

    // class GPUBufferToImageItem : public IGPUAsyncLoadItem {
    // private:
    //     Texture*                    image_;
    //     Buffer*                     stagingBuffer_;
    //     std::vector<ImageRegion>    regions_;
    //     CommandBuffer*              cb_;
    //     CommandQueue*               queue_;
    // public:
    //     GPUBufferToImageItem(
    //         Device* device,
    //         Fence* fence,
    //         Texture* image, 
    //         Buffer* staging, 
    //         ImageRegion const* regions, 
    //         uint32_t count, 
    //         CommandBuffer* cb, 
    //         CommandQueue* queue
    //     )
    //         : IGPUAsyncLoadItem(device, fence)
    //         , image_(image)
    //         , stagingBuffer_(staging)
    //         , regions_(regions, regions + count)
    //         , cb_(cb)
    //         , queue_(queue)
    //     {}

    //     virtual void onLoadComplete(CommandBuffer* cb) override;
    // };

}