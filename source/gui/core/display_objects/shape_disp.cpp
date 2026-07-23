#include "shape_disp.h"
#include "render/render_data.h"

namespace gui {

    // ---- internal helpers ----

    static void markDirty(entt::entity entity) {
        reg.emplace_or_replace<dispcomp::mesh_dirty>(entity);
        reg.emplace_or_replace<dispcomp::batch_dirty>(entity);
    }

    // ---- Shape parameter setters ----

    void ShapeDisp::setShapeType(dispcomp::shape_desc_t::Type type) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.type = type;
        markDirty(entity_);
    }

    void ShapeDisp::setLineSize(float lineSize) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.lineSize = lineSize;
        markDirty(entity_);
    }

    void ShapeDisp::setFillColor(Color4B const& color) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.fillColor = color;
        markDirty(entity_);
    }

    void ShapeDisp::setLineColor(Color4B const& color) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.lineColor = color;
        markDirty(entity_);
    }

    void ShapeDisp::setPolygonPoints(std::vector<glm::vec2> const& points) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.polygonPoints = points;
        markDirty(entity_);
    }

    void ShapeDisp::setSides(int sides) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.sides = sides;
        markDirty(entity_);
    }

    void ShapeDisp::setStartAngle(float angle) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.startAngle = angle;
        markDirty(entity_);
    }

    void ShapeDisp::setDistances(std::vector<float> const& distances) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.distances = distances;
        markDirty(entity_);
    }

    // ---- Convenience drawing methods ----

    void ShapeDisp::drawRect(float lineSize, Color4B lineColor, Color4B fillColor) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.type      = dispcomp::shape_desc_t::Type::Rect;
        sd.lineSize  = lineSize;
        sd.lineColor = lineColor;
        sd.fillColor = fillColor;
        markDirty(entity_);
    }

    void ShapeDisp::drawEllipse(float lineSize, Color4B lineColor, Color4B fillColor) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.type      = dispcomp::shape_desc_t::Type::Ellipse;
        sd.lineSize  = lineSize;
        sd.lineColor = lineColor;
        sd.fillColor = fillColor;
        markDirty(entity_);
    }

    void ShapeDisp::drawPolygon(std::vector<glm::vec2> const& points, Color4B fillColor) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.type          = dispcomp::shape_desc_t::Type::Polygon;
        sd.fillColor     = fillColor;
        sd.polygonPoints = points;
        markDirty(entity_);
    }

    void ShapeDisp::drawRegularPolygon(int sides, float lineSize, Color4B lineColor, Color4B fillColor, float startAngle, std::vector<float> const& distances) {
        auto& sd = reg.get_or_emplace<dispcomp::shape_desc_t>(entity_);
        sd.type      = dispcomp::shape_desc_t::Type::RegularPolygon;
        sd.sides     = sides;
        sd.lineSize  = lineSize;
        sd.lineColor = lineColor;
        sd.fillColor = fillColor;
        sd.startAngle = startAngle;
        sd.distances = distances;
        markDirty(entity_);
    }

    void ShapeDisp::markMeshDirty() {
        markDirty(entity_);
    }

}