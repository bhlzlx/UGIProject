#include "ui_render.h"
#include "render/render_data.h"
#include "ugi_types.h"
#include "ui_image_render.h"
#include "text_sdf_render.h"

namespace gui {

    ui_render_batches_t BuildImageRenderBatches(std::vector<void*> const& items, std::vector<item_args_t*> const& args, ugi::Texture* texture) {
        auto render = UIImageRender::Instance();
        std::vector<image_render_data_t> datas;
        for(size_t i = 0; i<items.size(); ++i) {
            datas.push_back({(image_mesh_t*)items[i], args[i]});
        }
        return render->buildImageRenderBatch(datas, texture);
    }

    ui_render_batches_t BuildTextRenderBatches(std::vector<void*> const& items, std::vector<item_args_t*> const& args, ugi::Texture* texture) {
        auto render = TextSDFRender::Instance();
        std::vector<image_render_data_t> datas;
        for(size_t i = 0; i<items.size(); ++i) {
            datas.push_back({(image_mesh_t*)items[i], args[i]});
        }
        return render->buildRenderBatch(datas, texture);
    }

    void DestroyRenderBatches(ui_render_batches_t const& batch) {
        switch(batch.type) {
            case gui::UIMeshType::Image: {
                auto render = UIImageRender::Instance();
                render->destroyRenderBatch(batch);
            }
            case UIMeshType::Font: {
                auto render = TextSDFRender::Instance();
                render->destroyRenderBatch(batch);
            }
            case UIMeshType::None:
            case UIMeshType::SubBatch:
              break;
            }
    }

    std::vector<FrameBatch> frameBatches;

    void ClearFrameBatchCache() {
        frameBatches.clear();
    }

    void CommitRenderBatch(ui_render_batches_t const& batch, glm::mat4 const& batchWorld) {
        frameBatches.push_back({batch, batchWorld});
    }

    void DrawRenderBatches(ugi::RenderCommandEncoder* encoder) {
        ugi::raster_state_t rasterizationState;
        rasterizationState.polygonMode = ugi::polygon_mode_t::Line;
        // rasterizationState.polygonMode = ugi::polygon_mode_t::Fill;
        for(auto& fb: frameBatches) {
            switch(fb.batch.type) {
                case gui::UIMeshType::Image: {
                    auto render = UIImageRender::Instance();
                    render->bind(encoder);
                    render->setRasterization(rasterizationState);
                    render->drawBatch(fb.batch, fb.batchWorld, encoder);
                    break;
                }
                case gui::UIMeshType::Font: {
                    auto render = TextSDFRender::Instance();
                    render->bind(encoder);
                    render->setRasterization(rasterizationState);
                    render->drawBatch(fb.batch, fb.batchWorld, encoder);
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
