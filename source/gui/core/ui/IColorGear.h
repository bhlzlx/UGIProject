#pragma once
#include "../declare.h"

namespace gui {
    class IColorGear {
    public:
        virtual void setColor(glm::vec3 const& color) = 0;
        virtual glm::vec3 getColor() const = 0;
    };
    class IOutlineColorGear : public IColorGear {
    public:
        virtual void setOutlineColor(glm::vec3 const& color) = 0;
        virtual glm::vec3 getOutlineColor() const = 0;
    };
}