#pragma once
#include <vector>
#include <string>
#include <unordered_map>
#include <core/declare.h>
#include <core/data_types/tween_types.h>
#include <core/data_types/tweener.h>
#include <core/ui/IColorGear.h>
#include "utils/byte_buffer.h"

namespace gui {

    class Controller;
    class Object;

    /// 参照 C# Gear 索引
    enum class GearIndex : int {
        Display     = 0,
        XY          = 1,
        Size        = 2,
        Look        = 3,
        Color       = 4,
        Animation   = 5,
        Text        = 6,
        Icon        = 7,
        Display2    = 8,
        FontSize    = 9,
    };

    using GearType = UnderlyingEnum<GearIndex>;

    struct GearColorValue {
        glm::vec3 color;
        glm::vec3 strokeColor;
    };

    /// Gear 基类
    class GearBase {
    protected:
        Object*             _owner = nullptr;
        Controller*         _controller = nullptr;
        TweenConfig*        _tweenConfig;
        bool                _tweenLocked = false;
        bool                _hasTween = false;
    public:
        static bool         disableAllTweenEffect;
        GearBase(Object* owner) : _owner(owner) {}
        virtual ~GearBase() = default;

        Controller* controller() const { return _controller; }
        void setController(Controller* c) { _controller = c; }

        TweenConfig const* tweenConfig() { return _tweenConfig; }

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
        GearDisplay(Object* owner);
        void setup(ByteBuffer& buffer) override;
        void apply() override;

        uint32_t addLock();
        void releaseLock(uint32_t token);
        bool connected() const;
    protected:
        void addStatus(std::string const&, ByteBuffer&) override {}
        void init() override;
    private:
        int          _visible = 0;
        uint32_t     _displayLockToken = 1;
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
    class GearColor : public GearBase, public ITweenListener {
        std::unordered_map<std::string, GearColorValue> _storage;
        GearColorValue _default = {};
        IColorGear*          _colorGear = nullptr;         // init() 中缓存，避免 dynamic_cast
        IOutlineColorGear*   _outlineColorGear = nullptr;
    public:
        GearColor(Object* owner) : GearBase(owner) {}
        void apply() override;
    protected:
        void addStatus(std::string const& page, ByteBuffer& buffer) override;
        void init() override;
        void onTweenStart(Tweener* tweener) override;
        void onTweenUpdate(Tweener* tweener) override;
        void onTweenComplete(Tweener* tweener) override;
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
    class GearDisplay2 : public GearBase {
    public:
        std::vector<std::string>   pages;
        int                        condition = 0;

        GearDisplay2(Object* owner);
        void setup(ByteBuffer& buffer) override;
        void apply() override;

        bool evaluate(bool connected);
    protected:
        void addStatus(std::string const&, ByteBuffer&) override {}
        void init() override;
    private:
        int     _visible = 0;
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
