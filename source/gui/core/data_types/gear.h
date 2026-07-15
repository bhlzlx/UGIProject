#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <core/declare.h>
#include <core/data_types/tween_types.h>
#include "utils/byte_buffer.h"

namespace gui {

    class Controller;
    class Object;

    struct GearColorValue {
        Color4B color;
        Color4B strokeColor;
    };

    /// Gear 基类
    class GearBase {
    protected:
        Object*             _owner = nullptr;
        Controller*         _controller = nullptr;
        TweenConfig         _tweenConfig;
        bool                _tweenLocked = false;
        bool                _hasTween = false;
    public:
        static bool         disableAllTweenEffect;
        GearBase(Object* owner) : _owner(owner) {}
        virtual ~GearBase() = default;

        Controller* controller() const { return _controller; }
        void setController(Controller* c) { _controller = c; }

        TweenConfig& tweenConfig() { return _tweenConfig; }
        TweenConfig const& tweenConfig() const { return _tweenConfig; }

        virtual void setup(ByteBuffer& buffer);
        virtual void apply() = 0;
        virtual void updateState() {}
    protected:
        virtual void addStatus(std::string const& page, ByteBuffer& buffer) = 0;
        virtual void init() = 0;
        void setupTween(ByteBuffer& buffer);
    };

    // ============= GearDisplay =============
    class GearDisplay : public GearBase {
    public:
        std::vector<std::string> pages;
        GearDisplay(Object* owner) : GearBase(owner) {}
        void setup(ByteBuffer& buffer) override;
        void apply() override;
    protected:
        void addStatus(std::string const&, ByteBuffer&) override {}
        void init() override {}
    };

    // ============= GearXY =============
    class GearXY : public GearBase {
        struct Value { float x, y, px, py; };
        std::unordered_map<std::string, Value> _storage;
        Value _default = {};
        bool _positionsInPercent = false;
    public:
        GearXY(Object* owner) : GearBase(owner) {}
        void setup(ByteBuffer& buffer) override;
        void apply() override;
        void updateState() override;
    protected:
        void addStatus(std::string const& page, ByteBuffer& buffer) override;
        void init() override;
    };

    // ============= GearSize =============
    struct GearSizeValue {
        float w, h;
        float scaleX = 1.0f;
        float scaleY = 1.0f;
    };

    class GearSize : public GearBase {
        std::unordered_map<std::string, GearSizeValue> _storage;
        GearSizeValue _default;
    public:
        GearSize(Object* owner) : GearBase(owner) {}
        void apply() override;
        void updateState() override;
    protected:
        void addStatus(std::string const& page, ByteBuffer& buffer) override;
        void init() override;
    };

    // ============= GearColor =============
    class GearColor : public GearBase {
        std::unordered_map<std::string, GearColorValue> _storage;
        GearColorValue _default = {};
    public:
        GearColor(Object* owner) : GearBase(owner) {}
        void apply() override;
    protected:
        void addStatus(std::string const& page, ByteBuffer& buffer) override;
        void init() override;
    };

    // ============= GearLook =============
    struct GearLookValue {
        float alpha = 1.0f;
        float rotation = 0;
        bool  grayed = false;
        bool  touchable = true;
    };

    class GearLook : public GearBase {
        std::unordered_map<std::string, GearLookValue> _storage;
        GearLookValue _default;
    public:
        GearLook(Object* owner) : GearBase(owner) {}
        void apply() override;
    protected:
        void addStatus(std::string const& page, ByteBuffer& buffer) override;
        void init() override;
    };

    // ============= GearText =============
    class GearText : public GearBase {
        std::unordered_map<std::string, std::string> _storage;
        std::string _default;
    public:
        GearText(Object* owner) : GearBase(owner) {}
        void apply() override;
    protected:
        void addStatus(std::string const& page, ByteBuffer& buffer) override;
        void init() override;
    };

    // ============= GearIcon =============
    class GearIcon : public GearBase {
        std::unordered_map<std::string, std::string> _storage;
        std::string _default;
    public:
        GearIcon(Object* owner) : GearBase(owner) {}
        void apply() override;
    protected:
        void addStatus(std::string const& page, ByteBuffer& buffer) override;
        void init() override;
    };

    // ============= GearDisplay2 =============
    // 编辑器的预览模式可见性，运行时行为和 GearDisplay 一样
    class GearDisplay2 : public GearDisplay {
    public:
        GearDisplay2(Object* owner) : GearDisplay(owner) {}
        void setup(ByteBuffer& buffer) override { GearDisplay::setup(buffer); }
    };

    // ============= GearFontSize =============
    class GearFontSize : public GearBase {
        std::unordered_map<std::string, float> _storage;
        float _default = 12.0f;
    public:
        GearFontSize(Object* owner) : GearBase(owner) {}
        void apply() override;
    protected:
        void addStatus(std::string const& page, ByteBuffer& buffer) override;
        void init() override;
    };

    /// 工厂函数
    GearBase* createGear(int index, Object* owner);

}
