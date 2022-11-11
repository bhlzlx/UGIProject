#include "AsyncLoadManager.h"
#include "GPUAsyncLoadItem.h"

namespace ugi {

    void GPUAsyncLoadManager::tick(CommandBuffer* cb) {
        std::unique_lock<std::mutex> lock(mutex_[workindex_]);
        for(auto iter = worklist_[workindex_].begin(); iter != worklist_[workindex_].end(); ++iter) {
            auto& item = *iter;
            if(item.isComplete()) {
                item.onLoadComplete(cb);
            } else {
                workSwapbuffer_.push_back(item);
            }
        }
        worklist_[workindex_].swap(workSwapbuffer_);
        workSwapbuffer_.clear();
        workindex_ = workindex_^0x1;
    }

    void GPUAsyncLoadManager::registerAsyncLoad(GPUAsyncLoadItem&& item) {
        unsigned int index = 0;
        while(true) {
            if(mutex_[index].try_lock()) {
                worklist_[index].push_back(std::move(item));
                mutex_[index].unlock();
                break;
            } else {
                index = index ^ 0x1;
            }
        }
    }
}