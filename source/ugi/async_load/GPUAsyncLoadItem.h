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
        Buffer* stagingBuffer_;
    public:
        GPUMeshAsyncLoadItem(Device* device, Fence* fence, Mesh* mesh, Buffer* stagingBuffer)
            : IGPUAsyncLoadItem(device, fence)
            , mesh_(mesh)
            , stagingBuffer_(stagingBuffer)
        {}
        virtual void onLoadComplete() override;
    };

}