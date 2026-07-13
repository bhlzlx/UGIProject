#include "event_dispatcher.h"
#include <algorithm>

namespace gui {

    // ============= EventContext =============

    EventContext::EventContext(Value event, EventDispatcher* sender, void* data)
        : event_(event)
        , sender_(sender->getHandle())
        , stopped_(0)
        , defaultPrevented_(0)
        , captureTouch_(1)
        , data_(data)
    {}

    EventContext::EventContext(std::string const& event, EventDispatcher* sender, void* data)
        : event_()
        , sender_(sender->getHandle())
        , stopped_(0)
        , defaultPrevented_(0)
        , captureTouch_(1)
        , data_(data)
        , strEvent_(event)
    {}

    // ============= EventBridge =============

    void EventBridge::remove(EventCallback const& cb) {
        // 按函数指针比较, std::function 不直接支持 ==, 用地址近似
        auto it = std::remove_if(dispatchers.begin(), dispatchers.end(),
            [&](EventCallback const& f) {
                return f.target_type() == cb.target_type();
            });
        dispatchers.erase(it, dispatchers.end());
    }

    void EventBridge::removeCapture(EventCallback const& cb) {
        auto it = std::remove_if(captures.begin(), captures.end(),
            [&](EventCallback const& f) {
                return f.target_type() == cb.target_type();
            });
        captures.erase(it, captures.end());
    }

    void EventBridge::fire(EventContext* ctx, bool capture) {
        auto& list = capture ? captures : dispatchers;
        for (auto& cb : list) {
            if (ctx->isStopped()) break;
            cb(ctx);
        }
    }

    // ============= EventDispatcher =============

    EventDispatcher::~EventDispatcher() {
        for (auto* item : callbackItems_) delete item;
    }

    EventBridge& EventDispatcher::getBridge(std::string const& type) {
        auto it = _bridges.find(type);
        if (it != _bridges.end()) return it->second;
        return _bridges[type];
    }

    // ---- Value-based (backward compat) ----

    void EventDispatcher::addEventListener(Value event, EventCallback const& callback, EventTag tag) {
        if (tag) {
            for (auto& item : callbackItems_) {
                if (item->event == event && item->tag == tag) {
                    item->callback = callback;
                    return;
                }
            }
        }
        auto* item = new EventCallbackItem{ callback, event, tag, 0 };
        callbackItems_.push_back(item);
    }

    void EventDispatcher::removeEventListener(Value event, EventTag tag) {
        for (auto it = callbackItems_.begin(); it != callbackItems_.end();) {
            auto* item = *it;
            if (item->event == event && (!tag || item->tag == tag)) {
                if (item->dispatching) {
                    item->callback = {};
                    ++it;
                } else {
                    delete item;
                    it = callbackItems_.erase(it);
                }
            } else { ++it; }
        }
    }

    void EventDispatcher::clearEventListeners() {
        for (auto it = callbackItems_.begin(); it != callbackItems_.end();) {
            auto* item = *it;
            if (item->dispatching) {
                item->callback = nullptr;
                ++it;
            } else {
                delete item;
                it = callbackItems_.erase(it);
            }
        }
        _bridges.clear();
    }

    bool EventDispatcher::hasEventListener(Value event, EventTag tag) const {
        for (auto* item : callbackItems_) {
            if (item->callback && item->event == event &&
                (!tag || item->tag == tag)) return true;
        }
        return false;
    }

    bool EventDispatcher::dispatchEvent(Value event, void* data, Value dataValue) {
        EventContext ctx(event, this, data);
        bool fired = false;
        for (auto* item : callbackItems_) {
            if (item->event == event && item->callback) {
                item->dispatching = 1;
                item->callback(&ctx);
                item->dispatching = 0;
                fired = true;
            }
        }
        cleanRetired();
        return fired;
    }

    void EventDispatcher::cleanRetired() {
        for (auto it = callbackItems_.begin(); it != callbackItems_.end();) {
            auto* item = *it;
            if (item->dispatching == 0 && !item->callback) {
                delete item;
                it = callbackItems_.erase(it);
            } else { ++it; }
        }
    }

    // ---- string-based ----

    void EventDispatcher::addEventListener(std::string const& type, EventCallback callback) {
        getBridge(type).add(std::move(callback));
    }

    void EventDispatcher::removeEventListener(std::string const& type, EventCallback callback) {
        auto it = _bridges.find(type);
        if (it != _bridges.end()) it->second.remove(callback);
    }

    void EventDispatcher::addCapture(std::string const& type, EventCallback callback) {
        getBridge(type).addCapture(std::move(callback));
    }

    void EventDispatcher::removeCapture(std::string const& type, EventCallback callback) {
        auto it = _bridges.find(type);
        if (it != _bridges.end()) it->second.removeCapture(callback);
    }

    bool EventDispatcher::hasEventListener(std::string const& type) const {
        auto it = _bridges.find(type);
        return it != _bridges.end() && !it->second.isEmpty();
    }

    bool EventDispatcher::dispatchEvent(std::string const& type, void* data) {
        EventContext ctx(type, this, data);
        return internalDispatch(type, &ctx);
    }

    bool EventDispatcher::internalDispatch(std::string const& type, EventContext* ctx) {
        auto it = _bridges.find(type);
        if (it == _bridges.end() || it->second.isEmpty()) return false;
        it->second.fire(ctx, false);  // bubble phase
        return true;
    }

    bool EventDispatcher::bubbleEvent(std::string const& type, void* data) {
        EventContext ctx(type, this, data);
        // base implementation: just dispatch on self
        return internalDispatch(type, &ctx);
    }

}
