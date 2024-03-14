#include "display_object.h"
#include "core/display_objects/display_components.h"
#include "core/display_objects/display_object.h"
#include "display_components.h"
#include "display_object_utility.h"

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

    void DisplayObject::setParent(DisplayObject val) {
        auto p = this->parent();
        if(p) {
            removeFromParent();
        }
        reg.emplace_or_replace<dispcomp::parent>(entity_, val);
    }

    void DisplayObject::addChild(DisplayObject child) {
        dispcomp::children* comp = nullptr;
        if(!reg.any_of<dispcomp::children>(entity_)) {
            comp = &reg.emplace<dispcomp::children>(entity_);
        } else {
            comp = &reg.get<dispcomp::children>(entity_);
        }
        comp->val.push_back(child);
        return;
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
            children.insert(children.begin() + index, child);
            child.setParent(*this);
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

    void DisplayObject::setPosition(glm::vec2 const& pos) {
        auto trans = reg.get_or_emplace<dispcomp::basic_transfrom>(entity_);
        trans.position = pos;
    }

    void DisplayObject::setSize(glm::vec2 const& val) {
        auto trans = reg.get_or_emplace<dispcomp::basic_transfrom>(entity_);
        trans.size = val;
    }

    void DisplayObject::setPivot(glm::vec2 const& val) {
        auto trans = reg.get_or_emplace<dispcomp::basic_transfrom>(entity_);
        trans.pivot = val;
    }

    void DisplayObject::setRotation(float val) {
        reg.emplace_or_replace<dispcomp::rotation>(entity_, val);
    }

    void DisplayObject::setSkew(glm::vec2 val) {
        reg.emplace_or_replace<dispcomp::skew>(entity_, val);
    }

    void DisplayObject::setScale(glm::vec2 val) {
        reg.emplace_or_replace<dispcomp::scale>(entity_, val);
    }

    void DisplayObject::setChildIndex(DisplayObject child, uint32_t index) {
        auto children = &getChildren(entity_)->val;
        auto iter = std::find(children->begin(), children->end(), child);
        if(iter == children->end()) {
            return;
        }
        children->erase(iter);
        if(index >= children->size()) {
            children->push_back(child);
        } else {
            children->insert(children->begin() + index, child);
        }
        markBatchDirty(entity_);
    }

}
