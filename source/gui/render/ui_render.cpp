#include "ui_render.h"
#include "render/render_data.h"
#include "ui_image_render.h"

namespace gui {

    std::vector<ui_render_batches_t> frameBatches;

    image_item_t* CreateImageItem(dispcomp::image_mesh const& mesh) {
        auto render = UIImageRender::Instance();
        return render->createImageItem(mesh.desc);
    }

    ui_render_batches_t BuildImageRenderBatches(std::vector<void*> const& items, std::vector<ui_inst_data_t*> const& args, ugi::Texture* texture) {
        auto render = UIImageRender::Instance();
        std::vector<image_render_data_t> datas;
        for(size_t i = 0; i<items.size(); ++i) {
            datas.push_back({(image_item_t*)items[i], args[i]});
        }
        return render->buildImageRenderBatch(datas, texture);
    }

    void DestroyRenderBatches(ui_render_batches_t const& batch) {
        // 其实这里最好写成延迟销毁，以后再改
        switch(batch.type) {
            case gui::RenderItemType::Image: {
                auto render = UIImageRender::Instance();
                render->destroyRenderBatch(batch);
            }
            case RenderItemType::None:
            case RenderItemType::Font:
            case RenderItemType::SubBatch:
              break;
            }
    }

    void CommitRenderBatch(ui_render_batches_t const& batch) {
        frameBatches.push_back(batch);
    }

    void DrawRenderBatches(ugi::RenderCommandEncoder* encoder) {
        for(auto& batch: frameBatches) {
            switch(batch.type) {
                case gui::RenderItemType::Image: {
                    auto render = UIImageRender::Instance();
                    render->drawBatch(batch, encoder);
                    break;
                }
                default: {
                }
            }
        }
    }


    void GuiTick() {
    }

}
