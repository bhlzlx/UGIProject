#include "g_graph.h"
#include "core/display_objects/display_components.h"
#include "core/display_objects/display_object.h"
#include "render/render_data.h"
#include "core/ui/component.h"
#include "utils/byte_buffer.h"

namespace gui {

    GGraph::GGraph() {
        type_ = ObjectType::Graph;
    }

    GGraph::~GGraph() = default;

    void GGraph::createDisplayObject() {
        Object::createDisplayObject();
        auto& gfx = reg.get_or_emplace<dispcomp::item_render_data>(dispobj_);
        gfx.meshData.type = UIMeshType::Image;
        gfx.args.colorPacked = 0xFFFFFFFF;  // white: let vertex colors pass through
    }

    void GGraph::setupBeforeAdd(ByteBuffer& buffer, int startPos) {
        Object::setupBeforeAdd(buffer, startPos);

        buffer.seekToBlock(startPos, ObjectBlocks::FillInfo);  // block 5 = shape data for GGraph
        int type = buffer.read<uint8_t>();
        if (type == 0)
            return;

        float lineSize    = (float)buffer.read<int>();
        Color4B lineColor = buffer.read<Color4B>();
        Color4B fillColor = buffer.read<Color4B>();
        bool roundedRect  = buffer.read<bool>();
        float cornerRadius[4] = {};
        if (roundedRect) {
            for (int i = 0; i < 4; ++i) {
                cornerRadius[i] = buffer.read<float>();
            }
        }

        setLineWidth(lineSize);
        setLineColor(lineColor);
        setFillColor(fillColor);

        if (type == 1) {  // rect
            if (roundedRect) {
                // TODO: rounded rect mesh not yet implemented — fall back to plain rect
            }
            drawRect();
        } else if (type == 2) {  // ellipse
            drawEllipse();
        } else if (type == 3) {  // polygon
            int cnt = buffer.read<int16_t>() / 2;
            std::vector<glm::vec2> pts(cnt);
            for (int i = 0; i < cnt; ++i) {
                pts[i].x = buffer.read<float>();
                pts[i].y = buffer.read<float>();
            }
            drawPolygon(pts);
        } else if (type == 4) {  // regular polygon
            int sides = buffer.read<int16_t>();
            float startAngle = buffer.read<float>();
            int distCnt = buffer.read<int16_t>();
            std::vector<float> distances;
            if (distCnt > 0) {
                distances.resize(distCnt);
                for (int i = 0; i < distCnt; ++i)
                    distances[i] = buffer.read<float>();
            }
            drawRegularPolygon(sides, startAngle, distances);
        }
    }

    void GGraph::setColor(Color4B val) {
        color_ = val;
        if (dispobj_) {
            auto& gfx = reg.get<dispcomp::item_render_data>(dispobj_);
            gfx.args.colorPacked = color_;
            auto& s = reg.get_or_emplace<dispcomp::args_need_sync>(dispobj_);
            s.mask |= dispcomp::Asm_Color;
        }
    }

    // ---- helper: write shape_desc_t via ShapeDisp and trigger mesh rebuild ----

    void GGraph::applyShapeDesc() {
        if (!dispobj_) return;
        auto sd = shapeDisp();
        switch (shapeType_) {
        case ShapeType::Rect:
            sd.drawRect(lineSize_, lineColor_, fillColor_);
            break;
        case ShapeType::Ellipse:
            sd.drawEllipse(lineSize_, lineColor_, fillColor_);
            break;
        case ShapeType::Polygon:
            sd.drawPolygon(polygonPoints_, fillColor_);
            break;
        case ShapeType::RegularPolygon:
            sd.drawRegularPolygon(sides_, lineSize_, lineColor_, fillColor_, startAngle_, distances_);
            break;
        default:
            break;
        }
    }

    // ---- Shape drawing ----

    void GGraph::drawRect() {
        shapeType_ = dispcomp::shape_desc_t::Type::Rect;
        applyShapeDesc();
    }

    void GGraph::drawEllipse() {
        shapeType_ = dispcomp::shape_desc_t::Type::Ellipse;
        applyShapeDesc();
    }

    void GGraph::drawPolygon(std::vector<glm::vec2> const& points) {
        shapeType_     = dispcomp::shape_desc_t::Type::Polygon;
        polygonPoints_ = points;
        applyShapeDesc();
    }

    void GGraph::drawRegularPolygon(int sides, float startAngle, std::vector<float> const& distances) {
        shapeType_  = dispcomp::shape_desc_t::Type::RegularPolygon;
        sides_      = sides;
        startAngle_ = startAngle;
        distances_  = distances;
        applyShapeDesc();
    }

    // ---- Placeholder ----

    void GGraph::replaceMe(Object* target) {
        auto* p = parent();
        if (!p || !target) return;

        target->setAlpha(alpha_);
        target->setRotation(rotation_);
        target->setVisible(visible_);
        target->setTouchable(touchable_);
        target->setGrayed(grayed_);
        target->setPosition(position_);
        target->setSize(size_);

        int index = -1;
        for (int i = 0; i < p->numChildren(); ++i) {
            if (p->getChildAt(i) == this) { index = i; break; }
        }
        if (index < 0) return;

        p->addChildAt(target, (uint32_t)index);
        target->relations().copyFrom(relations_);
        p->removeChild(this);
    }

    void GGraph::addBeforeMe(Object* target) {
        auto* p = parent();
        if (!p || !target) return;

        int index = -1;
        for (int i = 0; i < p->numChildren(); ++i) {
            if (p->getChildAt(i) == this) { index = i; break; }
        }
        if (index < 0) return;

        p->addChildAt(target, (uint32_t)index);
    }

    void GGraph::addAfterMe(Object* target) {
        auto* p = parent();
        if (!p || !target) return;

        int index = -1;
        for (int i = 0; i < p->numChildren(); ++i) {
            if (p->getChildAt(i) == this) { index = i; break; }
        }
        if (index < 0) return;

        p->addChildAt(target, (uint32_t)(index + 1));
    }

    void GGraph::setSize(Size2D<float> const& size) {
        Object::setSize(size);
        if (shapeType_ != dispcomp::shape_desc_t::Type::None) {
            shapeDisp().markMeshDirty();
        }
    }

}
