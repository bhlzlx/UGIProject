#pragma once
#include "core/data_types/value.h"
#include "core/display_objects/display_object.h"
#include "object_factory.h"
#include <cstdint>
#include <queue>
#include <core/declare.h>
#include <core/events/event_dispatcher.h>
#include <core/data_types/relation.h>
#include <utils/byte_buffer.h>

/**
 * @brief Anchor & Pivot
 * anchor跟对象的位置有关系，也就是对象局部坐标的原点。一般anchor是pivot，如果不设置pivot为anchor
 * pivot是变换的中心点
 */


/**
 * @brief UID 和 引用计数
 * uid 可以用来给脚本使用
 * 引用计数可以用来在C++层控制生命周期，C++层存储uid去访问会比较慢。
 */

namespace gui {

    using user_data_t = void*;

    class Object : public EventDispatcher {
        friend class ObjectFactory;
        friend class Component;
    protected:
        std::string     id_;
        std::string     name_;
        glm::vec3       position_;
        Size2D<float>   rawSize_;
        Size2D<float>   size_;
        Size2D<float>   minSize_;
        Size2D<float>   maxSize_;

        // std::vector<GearBase>
        PackageItem*    packageItem_;
        Point2D<float>  scale_;
        Point2D<float>  pivot_;
        glm::vec2       skew_;
        float           alpha_;
        float           rotation_;
        uint8_t         pivotAsAnchor_:1;
        uint8_t         visible_:1;
        uint8_t         touchable_:1;
        uint8_t         grayed_:1;
        uint8_t         finalGrayed_:1;
        uint8_t         underConstruct_:1;
    protected:
        uint8_t         internalVisible_:1;
        uint8_t         handlingController_:1;
        uint8_t         draggable_:1;
        uint8_t         focusable_:1;
        uint8_t         pixelSnapping_:1;
        Component*      parent_;
        Relations       relations_;
        SortingOrder    sortingOrder_;
        Group*          group_;
        float           sizePercentInGroup_;
        DisplayObject   dispobj_;

        Value           data_;
        //
    public:
        Object()
            : EventDispatcher()
            , position_{}
            , rawSize_{}
            , size_{}
            , scale_{}
            , pivot_{}
            , alpha_(1.0f)
            , rotation_(0.0f)
            , pivotAsAnchor_(0)
            , visible_(1)
            , grayed_(0)
            , finalGrayed_(0)
            , internalVisible_(1)
            , handlingController_(0)
            , draggable_(0)
            , focusable_(0)
            , pixelSnapping_(0)
            , relations_(this)
            , sortingOrder_(SortingOrder::Ascent)
            , group_(nullptr)
            , sizePercentInGroup_(1.0f)
        {}

        ~Object();

        void release();
protected:
        virtual void onInit();
        virtual void onSizeChanged();
        virtual void onScaleChanged();
        virtual void onGrayedChanged();
        virtual void onPositionChanged();
        virtual void onAlphaChanged();
        virtual void onVisibleChanged();
        virtual void onEnter();
        virtual void onExit();
        virtual void onControllerChanged(Controller* controller);

        virtual void setupBeforeAdd(ByteBuffer& buffer, int startPos = 0);
        virtual void setupAfterAdd(ByteBuffer& buffer, int startPos = 0);

        virtual void createDisplayObject();

        void internalSetParent(Component* comp);

        bool init();

        void updateGear(int index);
        void checkGearDisplay();
        void setSizeDirectly(Size2D<float> const& size);
    private:
    public:
        float x() const { return position_.x; }
        float y() const { return position_.y; }
        void setX(float v) { position_.x = v; }
        void setY(float v) { position_.y = v; }

        glm::vec2 position() const { return position_; }
        void setPosition( glm::vec3 const& val) { position_ = val; }

        void setSize(Size2D<float> const& size) {
            size_ = size;
        }

        float xMin() const;
        float yMin() const;
        void setXMin(float val);
        void setYMin(float val);

        bool pixelSnapping() const;
        void setPixelSnapping(bool val);

        float width() const { return size_.width; }
        float height() const { return size_.height; }

        void setWidth(float val) { size_.width = val; }
        void setHeight(float val) { size_.height = val; }

        void center(bool restraint = false);
        void makeFullScreen();

        glm::vec2 pivot() const { return pivot_; }
        void setPivot( glm::vec2 const& pivot, bool asAnchor = false);
        bool isPivotAsAnchor() const { return pivotAsAnchor_; }
        float scaleX() const { return scale_.x; }
        float scaleY() const { return scale_.y; }
        void setScaleX(float val) { scale_.x = val; }
        void setScaleY(float val) { scale_.y = val; }
        void setScale(float x, float y);

        float skewX() const { return skew_.x; }
        float skewY() const { return skew_.y; }
        void setSkewX(float val) { skew_.x = val; }
        void setSkewY(float val) { skew_.y = val; }
        void setSkew(float x, float y);

        float rotation() const { return rotation_; }
        void setRotation(float val) { rotation_ = val; }

        float alpha() const { return alpha_; }
        void setAlpha(float val);

        bool grayed() const;
        void setGrayed(bool val);

        bool visible() const { return visible_; }
        void setVisible(bool);

        bool touchable() const { return touchable_; }
        void setTouchable(bool val) { touchable_ = val; }

        DisplayObject getDisplayObject() const {
            return dispobj_;
        }

        SortingOrder sortingOrder() const { return sortingOrder_; }
        void setSortingOrder(SortingOrder val) { sortingOrder_ = val; }

        Group* group() const { return group_; }
        void SetGroup(Group* val) { group_ = val; }

        // virtual std::string const& text() const;
        // virtual void setText(std::string const& val) const;

        // virtual std::string const& icon() const;
        // virtual void setIcon(std::string const& val);

        std::string const& tooltips() const;
        void setTooltips(std::string const& val);

        user_data_t data() const { return data_; }
        void setData(user_data_t value) { data_ = value; }

        void handleControllerChanged(Controller* controller);

        virtual void constructFromResource() {};

    };

}