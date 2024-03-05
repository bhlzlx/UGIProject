#include "display_object.h"
#include "core/display_objects/display_components.h"
#include "core/display_objects/display_object.h"
#include "display_components.h"

namespace gui {

    entt::registry reg;

    DisplayObject DisplayObject::createDisplayObject() {
        auto entity = reg.create();
        return DisplayObject(entity);
    }

    DisplayObject DisplayObject::parent() const {
        if(reg.any_of<dispcomp::parent>(entity_)) {
            return reg.get<dispcomp::parent>(entity_).val;
        }
        return DisplayObject();
    }

    void DisplayObject::addChildAt(DisplayObject child, uint32_t index) {
        dispcomp::children* comp = nullptr;
        if(!reg.any_of<dispcomp::children>(entity_)) {
            comp = &reg.emplace<dispcomp::children>(entity_);
        } else {
            comp = &reg.get<dispcomp::children>(entity_);
        }
        std::vector<DisplayObject> &children = comp->val;
        int count = children.size();
        if(index > count) {
            return;
        }
        if(child.parent() == *this) { // 还是这个父控件
            auto iter = std::find(children.begin(), children.end(), child);
            if(iter != children.end()) {
                auto oldIndex = iter - children.begin();
                std::swap(children[oldIndex], children[index]);
            }
        } else {
            // child.remove
            // children.insert()
        }
    }

    void DisplayObject::removeChild(DisplayObject child) {
        if(!reg.any_of<dispcomp::children>(entity_)) {
            return;
        }
        auto &children = reg.get<dispcomp::children>(entity_).val;
        auto iter = std::find(children.begin(), children.end(), child);
        if(iter != children.end()) {
            children.erase(iter);
        }
    }

    void DisplayObject::removeChildAt(uint32_t index) {
        if(!reg.any_of<dispcomp::children>(entity_)) {
            return;
        }
        auto &children = reg.get<dispcomp::children>(entity_).val;
        if(index < children.size()) {
            children.erase(children.begin() + index);
        }
    }

    void DisplayObject::removeFromParent() {
        auto parent = this->parent();
        if(!parent) {
            return;
        }
        parent.removeChild(*this);
    }

    void setPosition(Point2D<float> const& pos) {
        return;
    }

}
