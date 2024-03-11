#include "transition.h"
#include "tweener.h"

namespace gui {

    Transition::Transition(Component* owner)
        : owner_(owner)
        , delayedCallDelegate_(Tweener::OnDelayedPlay)
        , checkAllDelegate_(Tweener::OnCheckAllComplete)
        , timeScale_(1.0f)
        , ignoreEngineTimeScale_(true)
    {}

    Transition::Transition(Transition &&trans)
        : name_(std::move(trans.name_))
        , owner_(trans.owner_)
        , items_(std::move(trans.items_))
        , completeFn_(std::move(trans.completeFn_))
        , delayedCallDelegate_(trans.delayedCallDelegate_)
        , checkAllDelegate_(trans.checkAllDelegate_)
        , options_(trans.options_)
        , totalTimes_(trans.totalTimes_)
        , totalTasks_(trans.totalTasks_)
        , autoPlayTime_(trans.autoPlayTime_)
        , ownerBaseX_(trans.autoPlayDelay_)
        , ownerBaseY_(trans.ownerBaseY_)
        , totalDuration_(trans.totalDuration_)
        , autoPlayDelay_(trans.autoPlayDelay_)
        , timeScale_(trans.timeScale_)
        , startTime_(trans.startTime_)
        , endTime_(trans.endTime_)
        , reversed_(trans.reversed_)
        , autoPlay_(trans.autoPlay_)
        , playing_(trans.playing_)
        , paused_(trans.paused_)
        , ignoreEngineTimeScale_(trans.ignoreEngineTimeScale_)
    {
    }

    void Transition::setup(ByteBuffer& buffer) {
        return;
    }

}