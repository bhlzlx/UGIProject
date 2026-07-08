#pragma once
#include "command_encoder/render_cmd_encoder.h"
#include <core/display_objects/display_components.h>

namespace gui {

    ui_render_batches_t BuildImageRenderBatches(std::vector<void*> const& items, std::vector<item_args_t*> const& args, ugi::Texture* texture);
    ui_render_batches_t BuildTextRenderBatches(std::vector<void*> const& items, std::vector<item_args_t*> const& args, ugi::Texture* texture);
    //
    void DestroyRenderBatches(ui_render_batches_t const& batch);

    struct FrameBatch {
        ui_render_batches_t batch;
        glm::mat4           batchWorld;
    };

    void ClearFrameBatchCache();
    void CommitRenderBatch(ui_render_batches_t const& batch, glm::mat4 const& batchWorld);

    void SetVPMat(glm::mat4 const& vp);

    void DrawRenderBatches(ugi::RenderCommandEncoder* encoder);
}