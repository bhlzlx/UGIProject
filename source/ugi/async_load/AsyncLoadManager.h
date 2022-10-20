#pragma once
#include <vector>
#include <mutex>
#include <atomic>

namespace ugi {

    class IGPUAsyncLoadItem;
    class GPUAsyncLoadManager {
    private:
        std::vector<IGPUAsyncLoadItem*>      worklist_[2];
        std::vector<IGPUAsyncLoadItem*>      workSwapbuffer_;
        std::mutex                           mutex_[2];
        std::atomic<unsigned int>            workindex_;
    public:
        GPUAsyncLoadManager()
            : worklist_{}
            , mutex_{}
            , workindex_(0)
        {}
        void tick();
        void registerAsyncLoad(IGPUAsyncLoadItem* item);
    };

}