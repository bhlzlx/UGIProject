#include "display_object_utility.h"
#include "core/display_objects/display_components.h"

namespace gui {

    bool isFinalVisible(entt::entity ett) {
        return reg.any_of<dispcomp::final_visible>(ett);
    }

    bool isBatchNode(entt::entity ett) {
        return reg.any_of<dispcomp::batch_node>(ett);
    }

    dispcomp::batch_data* getBatchData(entt::entity ett) {
        if(reg.any_of<dispcomp::batch_data>(ett)) {
            return &reg.get<dispcomp::batch_data>(ett);
        }
        return nullptr;
    }

    void setVisible(entt::entity ett, bool visible) {
        if(visible) {
            reg.emplace_or_replace<dispcomp::visible>(ett);
        } else {
            reg.remove<dispcomp::visible>(ett);
        }
    }

    bool isVisible(entt::entity ett) {
        return reg.any_of<dispcomp::visible>(ett);
    }

    void setFinalVisible(entt::entity ett, bool visible) {
        if(visible) {
            reg.emplace_or_replace<dispcomp::final_visible>(ett);
        } else {
            reg.remove<dispcomp::final_visible>(ett);
        }
    }

    dispcomp::children* getChildren(entt::entity ett) {
        dispcomp::children* children = nullptr;
        if(reg.any_of<dispcomp::children>(ett)) {
            children = & reg.get<dispcomp::children>(ett);
        }
        return children;
    }

    DisplayObject getParent(entt::entity ett) {
        if(reg.any_of<dispcomp::parent>(ett)) {
            return reg.get<dispcomp::parent>(ett).val;
        } else {
            return DisplayObject();
        }
    }

    NGraphics* getGraphics(entt::entity ett) {
        if(reg.any_of<NGraphics>(ett)) {
            return &reg.get<NGraphics>(ett);
        }
        return nullptr;
    }

    void markBatchDirty(entt::entity ett) {
        reg.emplace_or_replace<dispcomp::batch_dirty>(ett);
    }

    
}