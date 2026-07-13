#pragma once
#include "../declare.h"
#include <functional>
#include <vector>
#include <unordered_map>
#include <string>
#include <LightWeightCommon/utils/handle.h>
#include "core/data_types/value.h"

namespace gui {

    using namespace comm;

    class EventContext {
    private:
        Value               event_;
        Handle              sender_;
        uint8_t             stopped_    : 1;
        uint8_t             defaultPrevented_ : 1;
        uint8_t             captureTouch_ : 1;
        void*               data_ = nullptr;
        Value               dataValue_;
    public:
        EventContext(Value event, EventDispatcher* sender, void* data);
        EventContext(std::string const& event, EventDispatcher* sender, void* data);

        Value event() const { return event_; }
        std::string const& strEvent() const { return strEvent_; }
        void* data() const { return data_; }
        void stopPropagation() { stopped_ = 1; }
        bool isStopped() const { return stopped_; }

    private:
        std::string         strEvent_;   // 字符串类型事件
    };

    using EventCallback = std::function<void(EventContext* context)>;

    /// 单个事件类型的回调集合 (Capture + Bubble)
    struct EventBridge {
        std::vector<EventCallback> captures;
        std::vector<EventCallback> dispatchers;

        bool isEmpty() const { return captures.empty() && dispatchers.empty(); }
        void add(EventCallback const& cb) { dispatchers.push_back(cb); }
        void remove(EventCallback const& cb);
        void addCapture(EventCallback const& cb) { captures.push_back(cb); }
        void removeCapture(EventCallback const& cb);
        void fire(EventContext* ctx, bool capture);
    };

    class EventTag {
    private:
        size_t tag_;
    public:
        constexpr EventTag() : tag_(0) { }
        EventTag(size_t tag) : tag_(tag) { }
        EventTag(void* ptr) : tag_((size_t)ptr) { }
        operator size_t() const { return tag_; }
        bool operator ==(EventTag const& t) const { return tag_ == t.tag_; }
        bool operator !=(EventTag const& t) const { return tag_ != t.tag_; }
        operator bool() const { return tag_ != 0; }
    };

    class EventDispatcher {
        struct EventCallbackItem {
            EventCallback   callback;
            Value           event;
            EventTag        tag;
            int             dispatching;
        };
    private:
        ObjectHandle                            handle_;
        std::vector<EventCallbackItem*>         callbackItems_;    // Value-based (backward compat)
        std::unordered_map<std::string, EventBridge>  _bridges;

        bool internalDispatch(std::string const& type, EventContext* ctx);
        void cleanRetired();

    public:
        EventDispatcher() = default;
        virtual ~EventDispatcher();

        EventBridge& getBridge(std::string const& type);

        // ---- Value-based (backward compat) ----
        void addEventListener(Value event, EventCallback const& callback, EventTag tag = EventTag());
        void removeEventListener(Value event, EventTag tag = EventTag());
        void clearEventListeners();
        bool hasEventListener(Value event, EventTag tag = EventTag()) const;
        bool dispatchEvent(Value event, void* data = nullptr, Value dataValue = Value());

        // ---- string-based (FairyGUI 主线) ----
        void addEventListener(std::string const& type, EventCallback callback);
        void addEventListener(const char* type, EventCallback callback) { addEventListener(std::string(type), std::move(callback)); }
        void removeEventListener(std::string const& type, EventCallback callback);
        void addCapture(std::string const& type, EventCallback callback);
        void removeCapture(std::string const& type, EventCallback callback);
        bool hasEventListener(std::string const& type) const;
        bool dispatchEvent(std::string const& type, void* data = nullptr);
        bool dispatchEvent(const char* type, void* data = nullptr) { return dispatchEvent(std::string(type), data); }

        /// Bubble: 从当前对象沿父链向上冒泡，每层先 capture 再 dispatch
        virtual bool bubbleEvent(std::string const& type, void* data = nullptr);
        bool bubbleEvent(const char* type, void* data = nullptr) { return bubbleEvent(std::string(type), data); }

        Handle getHandle() { return handle_.handle(); }
    };

}
