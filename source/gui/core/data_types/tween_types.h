#pragma once
#include <core/declare.h>
#include <core/ease/ease_type.h>
#include <memory>

namespace gui {

    class Tweener;

    struct TValue {
        union {
            glm::vec4 val; 
            struct {
                double d;
                float x;
                float y;
            };
        };
        glm::vec2 vec2() const { return glm::vec2(val.x, val.y); }
        glm::vec3 vec3() const { return glm::vec3(val.x, val.y, val.z); };
        glm::vec4 vec4() const { return val; };
        Color4B color4B() const { return Color4B((uint8_t)val.x, (uint8_t)val.y, (uint8_t)val.z, (uint8_t)val.w); }
        void setVec2(glm::vec2 const& v) { val.x = v.x; val.y = v.y; }
        void setVec3(glm::vec3 const& v) { val.x = v.x; val.y = v.y; val.z = v.z; }
        void setVec4(glm::vec4 const& v) { val = v; }
        void setColor4B(Color4B const& v) { val = glm::vec4(v.r, v.g, v.b, v.a); }
        void reset() { val = {}; }
        //
        bool operator == (TValue const& rval) const {
            return val == rval.val;
        }
    };

    struct TweenConfig {
        bool        tween   = true;           // 参照 C# GearTweenConfig: 默认 true
        EaseType    easeType = EaseType::QuadOut;
        float       duration = 0.3f;         // 参照 C#: 默认 0.3s
        float       delay    = 0;            // 参照 C#: 默认 0
        int         repeat   = 0;
        bool        yoyo     = false;
        uint32_t    displayLockToken = 0;
        Tweener*    tweener = nullptr;
    };

}
    