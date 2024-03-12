#include "gui.h"
#include "core/display_objects/display_components.h"
#include "core/display_objects/display_object.h"
#include "core/ui/stage.h"
#include "entt/entity/fwd.hpp"
#include "render/render_data.h"
#include "render/ui_image_render.h"

namespace gui {

    void tick() {
        auto stage = Stage::Instance();
        auto root = stage->defaultRoot();
    }
    

#pragma region fastaccess_functions 

    bool getFinalVisible(entt::entity ett) {
        return reg.any_of<dispcomp::final_visible>(ett);
    }

    bool isBatchNode(entt::entity ett) {
        return reg.any_of<dispcomp::batch_node>(ett);
    }

    void setVisible(entt::entity ett, bool visible) {
        if(visible) {
            reg.emplace_or_replace<dispcomp::visible>(ett);
        } else {
            reg.remove<dispcomp::visible>(ett);
        }
    }

    bool getVisible(entt::entity ett) {
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

#pragma endregion

// #pragma region d

    void updateVisibleRecursive_(entt::entity ett, bool parentVisible) {
        bool finalVisible = parentVisible && getVisible(ett);
        setFinalVisible(ett, finalVisible);
        if(isBatchNode(ett)) { // 到batch node就不要再往下更新了
            return;
        }
        auto children = getChildren(ett);
        if(children) {
            for(auto child: children->val) {
                updateVisibleRecursive_(child, finalVisible);
            }
        }
    }

    void updateVisibleRecursive(entt::entity ett, bool changedVisible, bool parentVisible) {
        bool finalVisible = parentVisible && changedVisible;
        setVisible(ett, changedVisible);
        setFinalVisible(ett, finalVisible);
        if(reg.any_of<dispcomp::children>(ett)) {
            auto& children = reg.get<dispcomp::children>(ett).val;
            for(auto child: children) {
                updateVisibleRecursive_(ett, finalVisible);
            }
        }
    }

    void updateVisible() {
        auto stage = Stage::Instance();
        auto root = stage->defaultRoot();
        //
        reg.view<dispcomp::visible_changed>().each([](auto ett, dispcomp::visible_changed changedVisible) {
            bool isRoot = false;
            isRoot = reg.any_of<dispcomp::is_root>(ett);
            auto parent = getParent(ett);
            if(!isBatchNode(ett) && parent && isRoot) {  // 没有父控件，而且是脏的，现在不是更新它的时候
                return;
            } else {
                updateVisibleRecursive(ett, changedVisible.visible, getFinalVisible(parent));
            }
        });
    }


    void updateBatchNodes() {
        reg.view<dispcomp::batch_dirty, dispcomp::batch_node>().each([](entt::entity ett, dispcomp::batch_node& batchNode) {
            auto &children = getChildren(ett)->val;
            for(auto child: children) {
                
            }
        });
    }
    
}