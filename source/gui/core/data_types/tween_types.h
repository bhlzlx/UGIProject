#pragma once
#include <core/declare.h>
#include <core/ease/ease_type.h>

namespace gui {

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
        void setVec2(glm::vec2 const& v) { val.x = v.x; val.y = val.y; }
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
        float       duration;
        EaseType    easeType;
        int         repeat;
        bool        yoyo;
        //
        TValue*     startValue;
        TValue*     endValue;
    };

}
    