#include "relation.h"
#include <core/declare.h>
#include <core/ui_objects/object.h>

namespace gui {

    void RelationItem::setTarget(Object* ptr) {
        if(ptr != target_) {
            if(target_) {
                unregistEventListener((Object*)target_);
            }
            target_ = ptr;
            if(ptr) {
                registEventListener(ptr);
            }
        }
    }

    void RelationItem::add(RelationType type, bool usePercent) {
        if(type == RelationType::Size) {
            add(RelationType::Width, usePercent);
            add(RelationType::Height, usePercent);
            return;
        }
        for(auto& info: infos_) {
            if(info.type == type) {
                return;
            }
        }
        internalAdd(type, usePercent);
    }

    void RelationItem::internalAdd(RelationType type, bool usePercent) {
        assert(type != RelationType::Size && "must not be size type!");
        RelationInfo info;
        info.isPercent = usePercent;
        info.type = type;
        info.axis = (type <= RelationType::RightRight || type == RelationType::Width || (type >= RelationType::LeftExtLeft && type <= RelationType::RightExtRight)) ? Axis::X : Axis::Y;
        infos_.push_back(info);
        if (usePercent || type == RelationType::LeftCenter || type == RelationType::CenterCenter || type == RelationType::RightCenter || type == RelationType::TopMiddle || type == RelationType::MiddleMiddel || type == RelationType::BottomMiddle)  {
            dst_->setPixelSnapping(true);
        }
    }

    void RelationItem::remove(RelationType type) {
        if(type == RelationType::Size) {
            remove(RelationType::Width);
            remove(RelationType::Height);
            return;
        }
        for(auto iter = infos_.begin(); iter != infos_.end(); ++iter) {
            if(iter->type == type) {
                infos_.erase(iter);
                break;
            }
        }
    }

    void RelationItem::copyFrom(RelationItem const& src) {
        setTarget(src.target_);
        infos_.clear();
        for(auto& info : src.infos_) {
            infos_.push_back(info);
        }
    }

    /***
     * 控件的大小变化，反推变化后的位置
     * applyPivot是按pivot反推还是按控件的中心点反推
     * width 变化量
     * height 变化量
    */
    void RelationItem::applyOnSelfSizeChanged(float width, float height, bool applyPivot) {
        if(!target_ || infos_.empty()) {
            return;
        }
        glm::vec2 opo = dst_->position();
        glm::vec2 opi = dst_->pivot();
        for(auto& info: infos_) {
            switch (info.type) {
            case RelationType::CenterCenter: {
                dst_->setX(opo.x - (0.5f - (applyPivot?opi.x : 0)) * width);
                break;
            }
            case RelationType::RightCenter:
            case RelationType::RightLeft:
            case RelationType::RightRight: {
                break;
            }
                /* code */
                break;
            
            default:
                break;
            }
        }
    }


    bool RelationItem::isEmpty() const {
        return infos_.empty();
    }

    Relations::Relations(Object* owner)
        : handling_(nullptr)
        , owner_(owner)
        , items_{}
    {}

    Relations::~Relations() {
        clearAll();
    }

    void Relations::add(Object* target, RelationType type, bool usePercent) {
        for(auto it = items_.begin(); it != items_.end(); ++it) {
            auto item = *it;
            if(target == item->getTarget()) {
                // item->
            }
        }
    }


}