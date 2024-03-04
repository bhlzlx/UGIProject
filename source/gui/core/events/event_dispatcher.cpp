#include "event_dispatcher.h"

namespace gui {

    // namespace {
    //     comm::NamePool EventNamePool;
    // }

    // comm::Name GetEventName(char const* name) {
    //     auto rst = EventNamePool.getName(name);
    //     return rst;
    // }

    EventContext::EventContext(Value event, EventDispatcher* sender, void* data)
        : event_(event)
        , sender_(sender->getHandle())
        , stopped_(0)
        , defaultPrevented_(0)
        , captureTouch_(1)
        , data_(data)
    {}

    void EventDispatcher::addEventListener(Value event, EventCallback const& callback, EventTag tag) {
        if(tag) {
            for(auto &item: callbackItems_) {
                if(item->event == event && item->tag == tag) {
                    item->callback = callback;
                    return;
                }
            }
        }
        EventCallbackItem* item = new EventCallbackItem {
            callback,
            event,
            tag,
            0
        };
        callbackItems_.push_back(item);
    }

    void EventDispatcher::removeEventListener(Value event, EventTag tag) {
        for(auto it = callbackItems_.begin(); it != callbackItems_.end();) {
            auto& itemPtr= *it;
            auto tag = itemPtr->tag;
            if(itemPtr->event == event && (!tag || itemPtr->tag == tag)) {
                if(itemPtr->dispatching) {
                    itemPtr->callback = {};
                    ++it;
                } else {
                    delete itemPtr;
                    it = callbackItems_.erase(it);
                }
            }
        }
    }

    bool EventDispatcher::hasEventListener(Value event, EventTag tag) const {
        for(auto iter = callbackItems_.begin(); iter != callbackItems_.end(); ++iter) {
            auto item = *iter;
            if(item->callback) {
                if(item->event == event) {
                    if(item->tag) {
                        if(item->tag == tag) {
                            return true;
                        }
                    } else {
                        return true;
                    }
                }
            }
        }
        return false;
    }

    bool EventDispatcher::dispatchEvent(Value event, void* data, Value dataValue) {
        if(!callbackItems_.size()) {
            return false;
        }
        EventContext ctx(event, this, data);
        return true;
    }

    void EventDispatcher::clearEventListeners() {
        for(auto iter = callbackItems_.begin(); iter != callbackItems_.end(); ++iter) {
            auto item = *iter;
            if(item->dispatching) {
                item->callback = nullptr;
                ++iter;
            } else {
                delete item;
                iter = callbackItems_.erase(iter);
            }
        }
    }
}