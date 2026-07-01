#include "event_dispatcher.h"

namespace gui {

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
            if(itemPtr->event == event && (!tag || itemPtr->tag == tag)) {
                if(itemPtr->dispatching) {
                    itemPtr->callback = {};
                    ++it;
                } else {
                    delete itemPtr;
                    it = callbackItems_.erase(it);
                }
            } else {
                ++it;
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

        // 复制一份 callback 列表，防止在 dispatch 过程中列表被修改
        std::vector<EventCallbackItem*> itemsCopy = callbackItems_;
        bool result = false;
        for(auto item : itemsCopy) {
            if(item->event == event && item->callback) {
                item->dispatching = 1;
                item->callback(&ctx);
                item->dispatching = 0;
                result = true;
            }
        }

        // 清理在 dispatch 过程中被标记删除的 callback
        for(auto it = callbackItems_.begin(); it != callbackItems_.end();) {
            auto item = *it;
            if(item->dispatching == 0 && !item->callback) {
                delete item;
                it = callbackItems_.erase(it);
            } else {
                ++it;
            }
        }

        return result;
    }

    bool EventDispatcher::bubbleEvent(Value event, void* data, std::vector<uint8_t> const& buffer) {
        // 先 dispatch 自己
        dispatchEvent(event, data, Value());
        // bubble 到父节点需要 DisplayObject 链，暂不实现
        return false;
    }

    void EventDispatcher::clearEventListeners() {
        for(auto iter = callbackItems_.begin(); iter != callbackItems_.end();) {
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
