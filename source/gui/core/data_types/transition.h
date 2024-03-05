#pragma once
#include <string>
#include <core/declare.h>
#include "core/data_types/handle.h"
#include "core/data_types/tweener.h"
#include <core/data_types/tween_types.h>
#include <core/ease/ease_type.h>
#include <core/utility/enum_bits.h>

namespace gui {

    enum class TransitionActionType {
        Size,
        Scale,
        Pivot,
        Alpha,
        Rotation,
        Color,
        Animation,
        Visible,
        Sound,
        Transition,
        Shake,
        ColorFilter,
        Skew,
        Text,
        Icon,
        Unknown
    };

    using TransitionHook = std::function<void()>;
    using PlayCompleteCallback = std::function<void()>;

    struct TransitionItem {
        float                   time;
        Handle                  targetID;
        TransitionActionType    type;
        TweenConfig*            tweenConfig;
        std::string             label;
        void*                   tval;                  
        TransitionHook          hook;
    };

    typedef void(*OnTransitionCompleteFn)(Transition const*);

    class Transition {
    public:
        enum class option_t {
            IgnoreDisplayController = 1,
            AutoStopDisabled = 2,
            AutoStopAtEnd = 4,
        };
    private:
        std::string                         name_;
        Component*                          owner_;
        std::vector<TransitionItem>         items_;
        OnTransitionCompleteFn              completeFn_;
        Tweener::TweenCallback              delayedCallDelegate_;
        Tweener::TweenCallback              checkAllDelegate_;
        enum_bits<option_t>                 options_;
        int                                 totalTimes_;
        int                                 totalTasks_;
        int                                 autoPlayTime_; 

        float                               ownerBaseX_;
        float                               ownerBaseY_;
        float                               totalDuration_;
        float                               autoPlayDelay_;
        float                               timeScale_;
        float                               startTime_;
        float                               endTime_;

        bool                                reversed_;
        bool                                autoPlay_;
        bool                                playing_;
        bool                                paused_;
        bool                                ignoreEngineTimeScale_;

    public:
        Transition(Component* owner);
        Transition(Transition &&trans);

        ~Transition() {
        }

        void setup(ByteBuffer& buffer);

    private:
        void play_();
    };

}