#pragma once
#include <functional>
#include <core/declare.h>
#include <core/data_types/tween_types.h>
#include <LightWeightCommon/utils/handle.h>

namespace gui {

    using namespace comm;

    enum class TweenPropType {
        None,
        X,
        Y,
        Position,
        Width,
        Height,
        Size,
        ScaleX,
        ScaleY,
        Scale,
        Rotation,
        Alpha,
        Progress
    };

    enum class TweenValueType : uint8_t {
        None, 
        Float,
        Vec2,
        Vec3,
        Vec4,
        Color4B = Vec4,
        Double, // double这个应该可以去掉，感觉没什么必要，还要让数据多占一些空间
        Shake,
    };

    void SetObjectTweenProps(Object* target, TweenPropType type, TValue const& val);

    class Tweener {
    public:
        using TweenCallback = std::function<void(Tweener*)>;
        using TweenCallbackSimple = std::function<void()>;
    public:
        Tweener();
        ~Tweener();

        Tweener* setDelay(float val);
        Tweener* setDuration(float val);
        Tweener* setBreakpoint(float val);
        Tweener* setEase(EaseType type);
        Tweener* setEasePeriod(float val);
        Tweener* setEaseOvershootOrAmplitude(float val);
        Tweener* setRepeat(int repeat, bool yoyo = false);
        Tweener* setTimeScale(float value);
        Tweener* setSnapping(bool value);
        Tweener* setTargetAny(void* val);
        Tweener* setTarget(Handle uid);
        Tweener* setTarget(Handle uid, TweenPropType type);
        Tweener* setUserData(Userdata const& ud);
        Tweener* setPath(InterpoPath* path);
        Tweener* setUpdateCallback(TweenCallback const& callback);
        Tweener* setStartCallback(TweenCallback const& callback);
        Tweener* setCompleteCallback(TweenCallback const& callback);
        Tweener* setCompleteCallbackSimple(TweenCallbackSimple const& callback);
        Tweener* setPaused(bool paused);

        float getDelay() const;
        float getDuration() const;
        int getRepeat() const;
        Handle getTarget() const;
        float getNormalizedTime() const;
        bool isCompleted() const;
        bool allCompleted() const;
        void seek(float time);
        void kill(bool complete = false);
        Userdata const& getUserData() const;

        TValue startVal;
        TValue endVal;
        TValue deltaVal;
        TValue val;
    private:
        Tweener* _to(float start, float end, float duration);
        Tweener* _to(const glm::vec2& start, const glm::vec2& end, float duration);
        Tweener* _to(const glm::vec3& start, const glm::vec3& end, float duration);
        Tweener* _to(const glm::vec4& start, const glm::vec4& end, float duration);
        Tweener* _to(const Color4B& start, const Color4B& end, float duration);
        Tweener* _to(double start, double end, float duration);
        Tweener* _shake(const glm::vec2& start, float amplitude, float duration);

        void _init();
        void _reset();
        void _update(float dt);
        void update();
        void callStartCallback();
        void callUpdateCallback();
        void callCompleteCallback();

        // void*               target_;
        Handle           target_;
        // cocos2d::Ref* _refTarget;
        TweenPropType       propType_;
        UnderlyingEnum<TweenValueType>      valueType_;
        //
        uint8_t             killed_:1;
        uint8_t             paused_:1;
        uint8_t             yoyo_:1;
        uint8_t             snapping_:1;
        uint8_t             started_:1;
        uint8_t             ended_:3;
        //
        float               delay_;
        float               duration_;
        float               breakpoint_;
        float               easeOvershootOrAmplitude_;
        float               easePeriod_;
        float               timeScale_;
        float               elapsedTime_;
        float               normalizedTime_;
        int                 repeat_;
        EaseType            easeType_;
        Userdata            userdata_;
        InterpoPath*        path_;

        TweenCallback       onUpdate_;
        TweenCallback       onStart_;
        TweenCallback       onComplete_;
        TweenCallbackSimple onComplete0_;
    public:
        static TweenCallback OnDelayedPlay;
        static TweenCallback OnCheckAllComplete;
        static TweenCallback OnDelayedPlayItem;
    };


}