#include "display_object_utility.h"
#include "core/display_objects/display_components.h"
#include <glm/ext/matrix_transform.hpp>
#include <vector>

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

    dispcomp::item_render_data* getRenderResource(entt::entity ett) {
        if(reg.any_of<dispcomp::item_render_data>(ett)) {
            return &reg.get<dispcomp::item_render_data>(ett);
        }
        return nullptr;
    }

    void markBatchDirty(entt::entity ett) {
        reg.emplace_or_replace<dispcomp::batch_dirty>(ett);
    }

    // 实体自身的局部矩阵 (相对直接父节点)
    glm::mat4 buildLocalMatrix(entt::entity ett) {
        if (!reg.any_of<dispcomp::basic_transform>(ett)) {
            return glm::mat4(1.0f);
        }
        auto& transform = reg.get<dispcomp::basic_transform>(ett);
        glm::mat4 mat = glm::translate(glm::mat4(1.0f),
            glm::vec3(transform.position.x, transform.position.y, 0.0f));
        if (reg.any_of<dispcomp::rotation>(ett)) {
            auto& rot = reg.get<dispcomp::rotation>(ett);
            mat = glm::rotate(mat, rot.val, glm::vec3(0.0f, 0.0f, 1.0f));
        }
        if (reg.any_of<dispcomp::skew>(ett)) {
            auto& sk = reg.get<dispcomp::skew>(ett);
            if (sk.val.x != 0.0f || sk.val.y != 0.0f) {
                glm::mat4 shearMat(1.0f);
                shearMat[1][0] = sk.val.x;
                shearMat[0][1] = sk.val.y;
                mat = mat * shearMat;
            }
        }
        if (reg.any_of<dispcomp::scale>(ett)) {
            auto& sc = reg.get<dispcomp::scale>(ett);
            mat = glm::scale(mat, glm::vec3(sc.val.x, sc.val.y, 1.0f));
        }
        mat = glm::translate(mat, glm::vec3(
            -transform.pivot.x * transform.size.x,
            -transform.pivot.y * transform.size.y, 0.0f));
        return mat;
    }

    glm::mat4 accumulateLocalToBatch(entt::entity item, entt::entity batch) {
        std::vector<entt::entity> chain;
        auto curr = item;
        while (curr != batch && curr != entt::null) {
            chain.push_back(curr);
            curr = getParent(curr);
        }
        glm::mat4 mat(1.0f);
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            mat = mat * buildLocalMatrix(*it);
        }
        return mat;
    }

    void updateItemTransforms() {
        // 处理 Asm_Transform 标记：重算 local-to-batch 矩阵，写入 gfx.args.transfrom
        reg.view<dispcomp::args_need_sync, dispcomp::item_batch_info, dispcomp::item_render_data>().each([](entt::entity ett, dispcomp::args_need_sync& sync, dispcomp::item_batch_info& info, dispcomp::item_render_data& gfx) {
            if (!(sync.mask & dispcomp::Asm_Transform)) return;
            gfx.args.transfrom = accumulateLocalToBatch(ett, info.batchEntity);
            sync.mask &= ~dispcomp::Asm_Transform;
        });
    }

    void syncArgsToBatch(entt::entity entity) {
        if (!reg.any_of<dispcomp::item_batch_info>(entity) ||
            !reg.any_of<dispcomp::item_render_data>(entity) ||
            !reg.any_of<dispcomp::args_need_sync>(entity)) {
            return;
        }
        auto& info = reg.get<dispcomp::item_batch_info>(entity);
        if (info.batchIdx < 0) return;

        auto& gfx = reg.get<dispcomp::item_render_data>(entity);

        auto* batchData = getBatchData(info.batchEntity);
        if (!batchData || info.batchIdx >= (int)batchData->batches.size()) return;

        auto& batches_t = batchData->batches[info.batchIdx];
        int subIdx = info.instIndex / 512;
        int idxInSub = info.instIndex % 512;
        if (subIdx >= (int)batches_t.batches.size()) return;

        batches_t.batches[subIdx]->cachedArgs[idxInSub] = gfx.args;
        reg.remove<dispcomp::args_need_sync>(entity);
    }

    void syncDirtyArgs() {
        reg.view<dispcomp::args_need_sync>().each([](entt::entity entity, dispcomp::args_need_sync&) {
            syncArgsToBatch(entity);
        });
    }

}