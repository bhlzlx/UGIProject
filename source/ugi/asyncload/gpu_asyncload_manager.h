#pragma once
#include <vector>
#include <mutex>
#include <atomic>
#include <ugi_declare.h>
#include <asyncload/gpu_asyncload_item.h>

namespace ugi {

    class GPUAsyncLoadManager {
    private:
        std::vector<GPUAsyncLoadItem>       worklist_[2];
        std::vector<GPUAsyncLoadItem>       workSwapbuffer_;
        std::mutex                          mutex_[2];
        std::atomic<unsigned int>           workindex_;
    public:
        GPUAsyncLoadManager()
            : worklist_{}
            , workSwapbuffer_{}
            , mutex_{}
            , workindex_(0)
        {}
        void tick(CommandBuffer* cb);
        void registerAsyncLoad(GPUAsyncLoadItem&& item);
    };

}