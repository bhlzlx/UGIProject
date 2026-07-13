#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <core/declare.h>
#include "utils/byte_buffer.h"

namespace gui {

    class Controller;
    class Object;

    /// Gear 基类
    class GearBase {
    protected:
        Object*             _owner = nullptr;
        Controller*         _controller = nullptr;
        bool                _hasTween = false;
    public:
        GearBase(Object* owner) : _owner(owner) {}
        virtual ~GearBase() = default;

        Controller* controller() const { return _controller; }
        void setController(Controller* c) { _controller = c; }

        virtual void setup(ByteBuffer& buffer);
        virtual void apply() = 0;
        virtual void updateState() {}
    protected:
        virtual void addStatus(std::string const& page, ByteBuffer& buffer) = 0;
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
    };

    // ============= GearSize =============
    class GearSize : public GearBase {
        struct Value { float w, h; };
        std::unordered_map<std::string, Value> _storage;
        Value _default = {};
    public:
        GearSize(Object* owner) : GearBase(owner) {}
        void apply() override;
        void updateState() override;
    protected:
        void addStatus(std::string const& page, ByteBuffer& buffer) override;
    };

    // ============= GearColor =============
    class GearColor : public GearBase {
        std::unordered_map<std::string, Color4B> _storage;
        Color4B _default;
    public:
        GearColor(Object* owner) : GearBase(owner) {}
        void apply() override;
    protected:
        void addStatus(std::string const& page, ByteBuffer& buffer) override;
    };

    // ============= GearLook =============
    class GearLook : public GearBase {
        struct Value { float alpha, rotation, grayed; };
        std::unordered_map<std::string, Value> _storage;
        Value _default = {1.0f, 0, 0};
    public:
        GearLook(Object* owner) : GearBase(owner) {}
        void apply() override;
    protected:
        void addStatus(std::string const& page, ByteBuffer& buffer) override;
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
    };

    /// 工厂函数
    GearBase* createGear(int index, Object* owner);

}
