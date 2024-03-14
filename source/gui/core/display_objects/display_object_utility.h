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

    NGraphics* getGraphics(entt::entity ett);

}