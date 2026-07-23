#pragma once
#include <core/ui/object.h>
#include <core/ui/IColorGear.h>
#include <core/display_objects/display_components.h>
#include <core/display_objects/shape_disp.h>
#include <core/declare.h>
#include <vector>

namespace gui {

    /// GGraph — 图形对象，对应编辑器里的"图形"
    /// 1) 绘制简单图形（矩形、椭圆、多边形）
    /// 2) 占位用途（ReplaceMe / AddBeforeMe / AddAfterMe）
    class GGraph : public Object, public IColorGear {
    private:
        Color4B                     color_{0xFFFFFFFF};

        // shape state — mirrored to dispcomp::shape_desc_t via ShapeDisp
        using ShapeType = dispcomp::shape_desc_t::Type;
        ShapeType                   shapeType_ = ShapeType::None;
        float                       lineSize_  = 0;
        Color4B                     lineColor_{0xFFFFFFFF};
        Color4B                     fillColor_{0xFFFFFFFF};
        std::vector<glm::vec2>      polygonPoints_;
        int                         sides_     = 5;
        float                       startAngle_ = 0;
        std::vector<float>          distances_;

        ShapeDisp shapeDisp() const { return ShapeDisp(dispobj_); }

        void applyShapeDesc();

    public:
        GGraph();
        ~GGraph();

        virtual void createDisplayObject() override;
        virtual void setupBeforeAdd(ByteBuffer& buffer, int startPos = 0) override;

        // IColorGear
        Color4B getColor() const override { return color_; }
        void setColor(Color4B val) override;

        // ---- Shape drawing ----
        /// 画矩形（颜色和线宽请先用 setFillColor / setLineColor / setLineWidth 设置）
        void drawRect();
        /// 画椭圆（内切于当前控件尺寸；颜色和线宽请先用 setter 设置）
        void drawEllipse();
        /// 画多边形（颜色和线宽请先用 setter 设置）
        void drawPolygon(std::vector<glm::vec2> const& points);
        /// 画正多边形（颜色和线宽请先用 setter 设置；startAngle 单位为度）
        void drawRegularPolygon(int sides, float startAngle = 0, std::vector<float> const& distances = {});
        /// 设置线条颜色
        void setLineColor(Color4B val) { lineColor_ = val; applyShapeDesc(); }
        /// 设置线条宽度
        void setLineWidth(float val) { lineSize_ = val; applyShapeDesc(); }
        /// 设置填充颜色
        void setFillColor(Color4B val) { fillColor_ = val; applyShapeDesc(); }
        /// 设置正多边形边数
        void setSides(int val) { sides_ = val; applyShapeDesc(); }
        /// 设置正多边形起始角度（度）
        void setStartAngle(float val) { startAngle_ = val; applyShapeDesc(); }
        /// 设置正多边形各顶点距离系数（0~1），用于雷达图等
        void setDistances(std::vector<float> const& val) { distances_ = val; applyShapeDesc(); }

        // ---- Placeholder ----
        /// 在显示列表中用 target 替换自己
        void replaceMe(Object* target);
        /// 在显示列表中把自己前面插入 target
        void addBeforeMe(Object* target);
        /// 在显示列表中把自己后面插入 target
        void addAfterMe(Object* target);

        void setSize(Size2D<float> const& size) override;
    };

}
