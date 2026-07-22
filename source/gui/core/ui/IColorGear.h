#pragma once
#include <cstdint>
#include "../declare.h"

namespace gui {
    class IColorGear {
    public:
        virtual void setColor(Color4B color) = 0;
        virtual Color4B getColor() const = 0;
    };
    class IOutlineColorGear : public IColorGear {
    public:
        virtual void setOutlineColor(Color4B color) = 0;
        virtual Color4B getOutlineColor() const = 0;
    };
}