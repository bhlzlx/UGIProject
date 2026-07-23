#pragma once

#include "display_object.h"
#include "display_components.h"

namespace gui {
    
    class ShapeDisp : public DisplayObject {
    public:
        ShapeDisp(entt::entity ett) : DisplayObject(ett) {}

        // ---- Shape parameter setters ----
        void setShapeType(dispcomp::shape_desc_t::Type type);
        void setLineSize(float lineSize);
        void setFillColor(Color4B const& color);
        void setLineColor(Color4B const& color);
        void setPolygonPoints(std::vector<glm::vec2> const& points);
        void setSides(int sides);
        void setStartAngle(float angle);
        void setDistances(std::vector<float> const& distances);

        // ---- Convenience drawing methods ----
        void drawRect(float lineSize, Color4B lineColor, Color4B fillColor);
        void drawEllipse(float lineSize, Color4B lineColor, Color4B fillColor);
        void drawPolygon(std::vector<glm::vec2> const& points, Color4B fillColor);
        void drawRegularPolygon(int sides, float lineSize, Color4B lineColor, Color4B fillColor, float startAngle, std::vector<float> const& distances);

        // ---- Mark mesh dirty (e.g. when parent size changes) ----
        void markMeshDirty();
    };

}