#pragma once
#include <vector>
#include <core/declare.h>
#include <core/data_types/tweener.h>
#include <utils/singleton.h>

namespace gui {

    /// Tween 管理器 — 单例
    /// 负责 Tweener 的对象池、活跃列表维护、每帧更新
    class TweenManager : public comm::Singleton<TweenManager> {
        friend class comm::Singleton<TweenManager>;
    public:
        /// 创建或从池中复用 Tweener，自动加入活跃列表
        Tweener* createTween();

        /// 检查 target 上是否有活跃的指定类型 Tween
        bool isTweening(Handle target, TweenPropType propType) const;

        /// 杀掉 target 上所有/指定类型的 Tween
        bool killTweens(Handle target, TweenPropType propType, bool completed);

        /// 获取 target 上指定类型的 Tweener（无则返回 nullptr）
        Tweener* getTween(Handle target, TweenPropType propType) const;

        /// 每帧调用，遍历活跃 Tween 并更新
        /// dt 为 0 时自动用内部时钟计算帧间隔
        void update(float dt = 0);

        /// 清空对象池
        void clean();

        /// 活跃 Tween 总数
        size_t activeCount() const { return _activeCount; }

    private:
        TweenManager() = default;

        static constexpr size_t kInitCapacity = 30;

        std::vector<Tweener*>   _activeTweens;  // 活跃列表（可能有 nullptr 空洞）
        std::vector<Tweener*>   _pool;          // 对象池（killed 后回收）
        size_t                  _activeCount = 0;
    };

    /// GTween — 公共静态 API
    /// 用法: GTween::To(0, 100, 1.0f)->setTarget(obj)
    class GTween {
    public:
        static Tweener* To(float start, float end, float duration);
        static Tweener* To(glm::vec2 const& start, glm::vec2 const& end, float duration);
        static Tweener* To(glm::vec3 const& start, glm::vec3 const& end, float duration);
        static Tweener* To(glm::vec4 const& start, glm::vec4 const& end, float duration);
        static Tweener* To(Color4B const& start, Color4B const& end, float duration);
        static Tweener* To(double start, double end, float duration);
        static Tweener* Shake(glm::vec2 const& start, float amplitude, float duration);

        /// 便捷：杀掉 target 上所有 Tween
        static void Kill(Handle target) { TweenManager::Instance()->killTweens(target, TweenPropType::None, false); }

        /// 便捷：检查 target 是否有 Tween
        static bool IsTweening(Handle target) { return TweenManager::Instance()->isTweening(target, TweenPropType::None); }
    };

}
