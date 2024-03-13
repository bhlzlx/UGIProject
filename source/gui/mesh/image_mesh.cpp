#include "image_mesh.h"
#include "core/display_objects/display_components.h"
#include "render/render_data.h"
#include "render/ui_image_render.h"
#include "render/ui_render.h"

namespace gui {

    void updateImageMesh() {
        reg.view<dispcomp::mesh_dirty, dispcomp::final_visible, dispcomp::image_mesh>().each([](auto ett, dispcomp::image_mesh& imageMesh) {
            NGraphics* graphics = nullptr;
            if(!reg.any_of<NGraphics>(ett)) {
                graphics = &reg.emplace_or_replace<NGraphics>(ett);
            } else {
                graphics = &reg.get<NGraphics>(ett);
            }
            graphics->renderItem.type = RenderItemType::Image;
            if(graphics->renderItem.item) {
                delete (image_render_data_t*)graphics->renderItem.item;
            }
            glm::vec2  uvbackup[2]; 
            uvbackup[0] = imageMesh.desc.uv[0];
            uvbackup[1] = imageMesh.desc.uv[1];
            switch(imageMesh.ext.flip) {
            case FlipType::None:{
                break;
            }
            case FlipType::Hori: {
                std::swap(imageMesh.desc.uv[0].x, imageMesh.desc.uv[1].x);
                break;
            }
            case FlipType::Vert: {
                std::swap(imageMesh.desc.uv[0].y, imageMesh.desc.uv[1].y);
                break;
            }
            case FlipType::Both: {
                std::swap(imageMesh.desc.uv[0], imageMesh.desc.uv[1]);
                break;
            }
            }
            graphics->renderItem.item = CreateImageItem(imageMesh);
            imageMesh.desc.uv[0] = uvbackup[0];
            imageMesh.desc.uv[1] = uvbackup[1];
        });
    }
}