#include "ui_render.h"
#include "render/render_data.h"
#include "ugi_types.h"
#include "ui_image_render.h"

namespace gui {

    ui_render_batches_t BuildImageRenderBatches(std::vector<void*> const& items, std::vector<item_args_t*> const& args, ugi::Texture* texture) {
        auto render = UIImageRender::Instance();
        std::vector<image_render_data_t> datas;
        for(size_t i = 0; i<items.size(); ++i) {
            datas.push_back({(image_mesh_t*)items[i], args[i]});
        }
        return render->buildImageRenderBatch(datas, texture);
    }

    void DestroyRenderBatches(ui_render_batches_t const& batch) {
        switch(batch.type) {
            case gui::UIMeshType::Image: {
                auto render = UIImageRender::Instance();
                render->destroyRenderBatch(batch);
            }
            case UIMeshType::None:
            case UIMeshType::Font:
            case UIMeshType::SubBatch:
              break;
            }
    }

    std::vector<ui_render_batches_t> frameBatches;

    void ClearFrameBatchCache() {
        frameBatches.clear();
    }

    void CommitRenderBatch(ui_render_batches_t const& batch) {
        frameBatches.push_back(batch);
    }

    void DrawRenderBatches(ugi::RenderCommandEncoder* encoder) {
        ugi::raster_state_t rasterizationState;
        rasterizationState.polygonMode = ugi::polygon_mode_t::Fill;
        for(auto& batch: frameBatches) {
            switch(batch.type) {
                case gui::UIMeshType::Image: {
                    auto render = UIImageRender::Instance();
                    render->bind(encoder);
                    render->setRasterization(rasterizationState);
                    render->drawBatch(batch, encoder);
                    break;
                }
                default: {
                }
            }
        }
    }

    void SetVPMat(glm::mat4 const& vp) {
        auto render = UIImageRender::Instance();
        render->setVP(vp);
    }

}
