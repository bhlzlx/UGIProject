#pragma once
#include <ugi_types.h>
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

}