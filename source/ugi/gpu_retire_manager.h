#pragma once
#include <vector>
#include <functional>
#include <mutex>
#include <utils/singleton.h>

namespace ugi {

    /// <summary>
    /// GPU 资源延迟销毁管理器
    /// 使用环形多缓冲，保证资源在 GPU 完成渲染（MaxFlightCount 帧后）才被释放
    /// </summary>
    class GPURetireManager : public comm::Singleton<GPURetireManager> {
        friend class comm::Singleton<GPURetireManager>;
    public:
        static constexpr int kMaxFlightCount = 3;   // 与 swapchain image count 一致

    private:
        // kMaxFlightCount 个槽位，构成环形缓冲
        // ring_[i] 中的任务将在 (writeIndex + i + 1) 帧后被处理
        std::vector<std::function<void()>>   ring_[kMaxFlightCount];
        int                                  writeIndex_ = 0;
        std::mutex                           mutex_;

    private:
        GPURetireManager() = default;

    public:
        /// <summary>
        /// 投递一个待退役的资源销毁回调，默认延迟 kMaxFlightCount 帧后执行
        /// </summary>
        void retire(std::function<void()>&& callback) {
            std::lock_guard<std::mutex> lock(mutex_);
            ring_[writeIndex_].push_back(std::move(callback));
        }

        /// <summary>
        /// 每帧调用，推进环形指针，执行并清理到期槽位的所有回调
        /// </summary>
        void onFrameTick() {
            std::lock_guard<std::mutex> lock(mutex_);
            writeIndex_ = (writeIndex_ + 1) % kMaxFlightCount;
            auto& tasks = ring_[writeIndex_];
            for (auto& task : tasks) {
                task();
            }
            tasks.clear();
        }
    };

}
