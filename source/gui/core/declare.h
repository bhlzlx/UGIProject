#pragma once
#include "glm/detail/qualifier.hpp"
#include <cstdint>
#include <string>
#include <glm/glm.hpp>

// ugi headers
#include <ugi_declare.h>

namespace gui {

    template<class T>
    using Point2D = glm::vec<2, T, glm::defaultp>;

    template<class T>
    struct Size2D {
        T width;
        T height;
        operator glm::vec<2,T,glm::defaultp>() const {
            return glm::vec<2,T,glm::defaultp>(width, height);
        }
    };

    template<class T>
    struct Rect {
        Point2D<T>  base;
        Size2D<T>   size;
        T left() const { return base.x; };
        T right() const { return base.x + size.width; };
        T top() const { return base.y; };
        T bottom() const { return base.y + size.height; };
    };

    struct Color4B {
        union {
            struct {
            #if BIG_ENDIAN
                uint8_t r, g, b, a; // #rgba
            #else
                uint8_t a, b, g, r; // #rgba
            #endif
            };
            uint32_t val;
        };
        Color4B() : val(0xffffffff) {}
        Color4B(uint32_t ival) : val(ival) {}
        Color4B(uint8_t rr, uint8_t gg, uint8_t bb, uint8_t aa)
        #if BIG_ENDIAN
            : r(rr), g(gg), b(bb), a(aa)
        #else
            : a(rr), b(gg), g(bb), r(aa)
        #endif
        {}
        operator uint32_t () const {
            return val;
        }
    };

    struct Color3B {
        #if BIG_ENDIAN
            uint8_t r,g,b;
        #else
            uint8_t b,g,r;
        #endif
        Color3B(uint32_t ival) {
            #if BIG_ENDIAN
                memcpy(this, (uint8_t*)&ival+1, 3);
            #else
                memcpy(this, &ival, 3);
            #endif
        }
        Color3B(uint8_t rr, uint8_t gg, uint8_t bb)
        #if BIG_ENDIAN
            :r(rr), g(gg), b(bb)
        #else
            :b(rr), g(gg), r(bb)
        #endif
        {}
        operator uint32_t () const {
            uint32_t val = 0xffffffff;
            #if BIG_ENDIAN
                memcpy((uint8_t*)&val+1, this, 3);
            #else
                memcpy(&val,this,3);
            #endif
            return val;
        }
    };

    enum class PackageItemType : uint8_t {
        Image, // 纹理
        MovieClip, // 序列
        Sound, // 音频
        Component, // 组件
        Atlas,
        Font,
        Swf,
        Misc,
        Unknown,
        Spine,
        DragonBones,
    };

    enum class ObjectType : uint8_t {
        Image,
        MovieClip,
        Swf,
        Graph,
        Loader,
        Group,
        Text,
        RichText,
        InputText,
        Component,
        List,
        Label,
        Button,
        ComboBox,
        ProgressBar,
        Slider,
        ScrollBar,
        Tree,
        Loader3D,
        Root,
    };
    

    enum class MouseButton : int {
        Left,
        Right,
        Middle,
        Invalid,
    };

    enum class SortingOrder {
        Ascent,
        Descent,
    };

    enum class RelationType {
        LeftLeft,
        LeftCenter,
        LeftRight,
        CenterCenter,
        RightRight,
        RightCenter,
        RightLeft,
        Width,
        LeftExtLeft,
        LeftExtRight,
        RightExtLeft,
        RightExtRight,
        //
        TopTop,
        TopMiddle,
        TopBotton,
        MiddleMiddel,
        BottomTop,
        BottomMiddle,
        BottomBottom,
        //
        Height,
        //
        TopExtTop,
        TopExtBottom,
        BottomExtTop,
        BottomExtBottom,
        Size,
    };

    enum class Axis {
        X,
        Y
    };

    enum class ImageScaleOption {
        None,
        Grid9,
        Tile,
    };

    class GUIContext;
    class Package;
    class PackageItem;
    class Group;
    class Touch;
    class InputEvent;
    class EventDispatcher;
    // ui object
    class Object;
    class Component;
    // display object
    class Container;
    class ScrollPane;

    ///
    class Transition;
    class Controller;
    class InputHandler;
    class ObjectDestroyManager;
    class ObjectTable;
    class ByteBuffer;
    class InterpoPath;

    struct AtlasSprite {
        PackageItem*    item;
        Rect<float>     rect;
        glm::vec2       origSize;
        glm::vec2       offset;
        bool            rotated;
    };

    struct Userdata {
    };

    template<class T>
    class UnderlyingEnum {
    public:
        using val_type = typename std::underlying_type<T>::type;
    private:
        T val_;
    public:
        UnderlyingEnum(T val = 0) : val_(val) {}
        UnderlyingEnum(val_type val) : val_((T)val) {}
        operator val_type() const { return (val_type)val_; }
        operator T() const { return val_; }
    };

    using Texture = ugi::Texture;
    using csref = std::string const&;

    GUIContext* GetGUIContext();

    ugi::StandardRenderContext* GetRenderContext();

}