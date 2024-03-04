#pragma once
#include "../events/event_dispatcher.h"
#include <core/declare.h>
#include <vector>
#include "utils/byte_buffer.h"

namespace gui {

    struct RelationInfo {
        RelationType    type;
        bool            isPercent;
        Axis            axis;
    };

    class RelationItem {
    private:
        Object*                         dst_;         // 被布局影响的对象
        Object*                         target_;        // 布局系统监听的对象
        std::vector<RelationInfo>       infos_;
        Point2D<float>                  targetPos_;
        Size2D<float>                   targetSize_;
    public:
        void setTarget(Object* ptr);
        Object* getTarget() const {
            return target_;
        }
        Object* getOwner() const {
            return dst_;
        }

        void add(RelationType type, bool usePercent);
        void internalAdd(RelationType type, bool usePercent);
        void remove(RelationType type);
        void copyFrom(RelationItem const& src);
        bool isEmpty() const;
        void applyOnSelfSizeChanged(float width, float height, bool applyPivot);
    private:
        void applyOnXYChanged(Object* target, RelationInfo const& info, glm::vec2 diff);
        void applyOnSizeChanged(Object* target, RelationInfo const& info);
        void registEventListener(Object* target);
        void unregistEventListener(Object* target);

        void onTargetXYChanged(EventContext* context);
        void onTargetSizeChanged(EventContext* context);
    };

    class Relations {
    private:
        Object*                     handling_;
        Object*                     owner_;
        std::vector<RelationItem*>  items_;
    public:
        Relations(Object* owner);
        ~Relations();

        void add(Object* target, RelationType type, bool usePercent = false);
        void remove(Object* target, RelationType* type);
        bool contains(Object* target);
        void clearFor(Object* target);
        void clearAll();
        void copyFrom(Relations const& other);
        void onOwnSizeChanged(glm::vec2 size, bool applyPivot);
        bool isEmpty() const;
        void setup(ByteBuffer buffer, bool parentToChild);
        Object* handling() const { return handling_; }
    };

}