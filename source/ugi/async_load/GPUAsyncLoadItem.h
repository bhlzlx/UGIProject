#pragma once
#include <UGITypes.h>
#include <vector>

namespace ugi {

    class Mesh;

    class IGPUAsyncLoadItem {
    protected:
        Device* device_;
        Fence* fence_;
    public:
        IGPUAsyncLoadItem(Device* device, Fence* fence) : device_(device), fence_(fence) {}
        bool isComplete();
        virtual ~IGPUAsyncLoadItem() {}
        virtual void onLoadComplete() = 0;
    };

    class GPUMeshAsyncLoadItem : public IGPUAsyncLoadItem {
    private:
        Mesh* mesh_;
        std::vector<Buffer*> stagingBuffers_;
    public:
        GPUMeshAsyncLoadItem(Device* device, Fence* fence, Mesh* mesh, std::vector<Buffer*>&& stagingBuffers)
            : IGPUAsyncLoadItem(device, fence)
            , mesh_(mesh)
            , stagingBuffers_(std::move(stagingBuffers))
        {}
        virtual void onLoadComplete() override;
    };

}