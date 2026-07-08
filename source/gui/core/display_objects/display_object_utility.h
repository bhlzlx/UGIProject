#pragma once

#include "core/display_objects/display_object.h"
#include "display_components.h"
#include <render/render_data.h>

namespace gui {

    void setVisible(entt::entity ett, bool visible);

    bool isVisible(entt::entity ett);

    void setFinalVisible(entt::entity ett, bool visible);

    bool isFinalVisible(entt::entity ett);

    bool isBatchNode(entt::entity ett);

    void markBatchDirty(entt::entity ett);

    dispcomp::batch_data* getBatchData(entt::entity ett);

    dispcomp::children* getChildren(entt::entity ett) ;

    DisplayObject getParent(entt::entity ett);

    dispcomp::item_render_data* getRenderResource(entt::entity ett);

    void syncArgsToBatch(entt::entity entity);
    void syncDirtyArgs();

    /// 处理所有需要重算 local-to-batch 矩阵的 item，更新 gfx.args.transfrom
    void updateItemTransforms();

    glm::mat4 buildLocalMatrix(entt::entity ett);
    glm::mat4 accumulateLocalToBatch(entt::entity item, entt::entity batch);

}