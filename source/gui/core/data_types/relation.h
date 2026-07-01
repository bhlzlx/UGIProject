#pragma once
#include "../events/event_dispatcher.h"
#include <core/declare.h>
#include <vector>
#include <glm/glm.hpp>
#include "utils/byte_buffer.h"

namespace gui {

    struct RelationInfo {
        RelationType    type;
        bool            isPercent;
        Axis            axis;
    };

    class RelationItem {
    private:
        Object*                         owner_;         // 被布局影响的对象 (dst)
        Object*                         target_;        // 布局系统监听的对象
        std::vector<RelationInfo>       infos_;
        Point2D<float>                  targetPos_;     // 缓存 target 的上次位置
        Size2D<float>                   targetSize_;    // 缓存 target 的上次大小
    public:
        RelationItem(Object* owner);

        Object* owner() const { return owner_; }

        void setTarget(Object* ptr);
        Object* getTarget() const {
            return target_;
        }

        void add(RelationType type, bool usePercent);
        void internalAdd(RelationType type, bool usePercent);
        void remove(RelationType type);
        void copyFrom(RelationItem const& src);
        void dispose();
        bool isEmpty() const;
        void applyOnSelfSizeChanged(float dWidth, float dHeight, bool applyPivot);
    private:
        void applyOnXYChanged(RelationInfo const& info, float dx, float dy);
        void applyOnSizeChanged(RelationInfo const& info);
        void addRefTarget(Object* target);
        void releaseRefTarget(Object* target);

        void onTargetXYChanged(EventContext* context);
        void onTargetSizeChanged(EventContext* context);
    };

    class Relations {
    private:
        Object*                     owner_;
        std::vector<RelationItem*>  items_;
    public:
        Object*                     handling = nullptr;

        Relations(Object* owner);
        ~Relations();

        void add(Object* target, RelationType type, bool usePercent = false);
        void remove(Object* target, RelationType type);
        bool contains(Object* target) const;
        void clearFor(Object* target);
        void clearAll();
        void copyFrom(Relations const& other);
        void onOwnerSizeChanged(float dWidth, float dHeight, bool applyPivot);
        bool isEmpty() const;
        void setup(ByteBuffer& buffer, bool parentToChild);
    };

}
