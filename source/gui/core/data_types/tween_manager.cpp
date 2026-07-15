#include "tween_manager.h"
#include <cassert>
#include <chrono>

namespace gui {

    Tweener* TweenManager::createTween() {
        Tweener* tweener;
        if (!_pool.empty()) {
            tweener = _pool.back();
            _pool.pop_back();
        } else {
            tweener = new Tweener();
        }
        tweener->_init();

        // 加入活跃列表
        _activeTweens.push_back(tweener);
        _activeCount++;

        return tweener;
    }

    bool TweenManager::isTweening(Handle target, TweenPropType propType) const {
        if (!target) return false;

        bool anyType = (propType == TweenPropType::None);
        for (size_t i = 0; i < _activeTweens.size(); ++i) {
            auto* t = _activeTweens[i];
            if (t && t->target_ == target && !t->killed_
                && (anyType || t->propType_ == propType)) {
                return true;
            }
        }
        return false;
    }

    bool TweenManager::killTweens(Handle target, TweenPropType propType, bool completed) {
        if (!target) return false;

        bool flag = false;
        bool anyType = (propType == TweenPropType::None);
        for (auto* t : _activeTweens) {
            if (t && t->target_ == target && !t->killed_
                && (anyType || t->propType_ == propType)) {
                t->kill(completed);
                flag = true;
            }
        }
        return flag;
    }

    Tweener* TweenManager::getTween(Handle target, TweenPropType propType) const {
        if (!target) return nullptr;

        bool anyType = (propType == TweenPropType::None);
        for (auto* t : _activeTweens) {
            if (t && t->target_ == target && !t->killed_
                && (anyType || t->propType_ == propType)) {
                return t;
            }
        }
        return nullptr;
    }

    void TweenManager::update(float dt) {
        // 自动计时
        static auto s_lastTime = std::chrono::steady_clock::now();
        if (dt <= 0) {
            auto now = std::chrono::steady_clock::now();
            dt = std::chrono::duration<float>(now - s_lastTime).count();
            s_lastTime = now;
            // 首帧或超长帧保护
            if (dt <= 0 || dt > 0.5f) dt = 0.016f;
        }

        if (_activeCount == 0) return;

        // 遍历更新 + 回收 killed 的 → 对象池
        // 使用双指针压缩空洞，避免每次 erase（O(n²)）
        size_t writePos = 0;
        for (size_t i = 0; i < _activeTweens.size(); ++i) {
            auto* t = _activeTweens[i];
            if (!t) continue;

            if (t->killed_) {
                // 回收进池
                t->_reset();
                _pool.push_back(t);
                _activeTweens[i] = nullptr;
                _activeCount--;
            } else {
                // 检查 target 是否已销毁
                if (t->target_) {
                    if (!t->target_.as<void*>()) {
                        t->killed_ = true;
                        t->_reset();
                        _pool.push_back(t);
                        _activeTweens[i] = nullptr;
                        _activeCount--;
                        continue;
                    }
                }

                if (!t->paused_) {
                    t->_update(dt);
                }

                // 更新后可能被 kill
                if (t->killed_) {
                    t->_reset();
                    _pool.push_back(t);
                    _activeTweens[i] = nullptr;
                    _activeCount--;
                    continue;
                }

                // 压缩：把存活的移到前面
                if (writePos != i) {
                    _activeTweens[writePos] = t;
                    _activeTweens[i] = nullptr;
                }
                writePos++;
            }
        }

        // 截断尾部空洞
        if (writePos < _activeTweens.size()) {
            _activeTweens.resize(writePos);
        }
    }

    void TweenManager::clean() {
        for (auto* t : _pool) {
            delete t;
        }
        _pool.clear();
    }

    // ==================== GTween ====================

    Tweener* GTween::To(float start, float end, float duration) {
        return TweenManager::Instance()->createTween()->_to(start, end, duration);
    }

    Tweener* GTween::To(glm::vec2 const& start, glm::vec2 const& end, float duration) {
        return TweenManager::Instance()->createTween()->_to(start, end, duration);
    }

    Tweener* GTween::To(glm::vec3 const& start, glm::vec3 const& end, float duration) {
        return TweenManager::Instance()->createTween()->_to(start, end, duration);
    }

    Tweener* GTween::To(glm::vec4 const& start, glm::vec4 const& end, float duration) {
        return TweenManager::Instance()->createTween()->_to(start, end, duration);
    }

    Tweener* GTween::To(Color4B const& start, Color4B const& end, float duration) {
        return TweenManager::Instance()->createTween()->_to(start, end, duration);
    }

    Tweener* GTween::To(double start, double end, float duration) {
        return TweenManager::Instance()->createTween()->_to(start, end, duration);
    }

    Tweener* GTween::Shake(glm::vec2 const& start, float amplitude, float duration) {
        return TweenManager::Instance()->createTween()->_shake(start, amplitude, duration);
    }

}
