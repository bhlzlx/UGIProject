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

    item_resource_t* getRenderResource(entt::entity ett) {
        if(reg.any_of<item_resource_t>(ett)) {
            return &reg.get<item_resource_t>(ett);
        }
        return nullptr;
    }

    void markBatchDirty(entt::entity ett) {
        reg.emplace_or_replace<dispcomp::batch_dirty>(ett);
    }

    void syncArgsToBatch(entt::entity entity) {
        if (!reg.any_of<dispcomp::item_batch_info>(entity) ||
            !reg.any_of<item_resource_t>(entity)) {
            return;
        }
        auto& info = reg.get<dispcomp::item_batch_info>(entity);
        if (info.batchIdx < 0) return;

        auto* batchData = getBatchData(info.batchEntity);
        if (!batchData || info.batchIdx >= (int)batchData->batches.size()) return;

        auto& batches_t = batchData->batches[info.batchIdx];
        int subIdx = info.instIndex / 512;
        int idxInSub = info.instIndex % 512;
        if (subIdx >= (int)batches_t.batches.size()) return;

        batches_t.batches[subIdx]->cachedArgs[idxInSub] =
            reg.get<item_resource_t>(entity).args;
    }

    void syncDirtyArgs() {
        reg.view<dispcomp::args_dirty>().each([](entt::entity entity) {
            syncArgsToBatch(entity);
            reg.remove<dispcomp::args_dirty>(entity);
        });
    }

}