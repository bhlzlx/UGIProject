#include "renderable.h"
#include <render_components/mesh.h>
#include <pipeline.h>
#include <command_buffer.h>
#include <vulkan_function_declare.h>
#include <ugi/render_components/pipeline_material.h>

namespace ugi {
    

    Renderable::Renderable(Mesh* mesh, Material* mtr, GraphicsPipeline* pipeline, raster_state_t const& rss)
        : mesh_(mesh)
        , material_(mtr)
        , pipeline_(pipeline)
        , rasterState_(rss)
    {
    }

    bool Renderable::prepared() const {
        return mesh_->prepared();
    }

    Mesh const* Renderable::mesh() const {
        return mesh_;
    }

    Material* Renderable::material() const {
        return material_;
    }

    raster_state_t Renderable::rasterState() const {
        return rasterState_;
   }

    GraphicsPipeline const* Renderable::pipeline() const {
        return pipeline_;
    }

    void Renderable::draw(RenderCommandEncoder* encoder) {
        mesh_->bind(encoder);
        pipeline_->applyMaterial(material_);
        pipeline_->setRasterizationState(rasterState_);
        pipeline_->bind(encoder);
        // vkCmdDrawIndexed(*encoder->commandBuffer(), mesh_->indexCount(), 1, 0, )
    }

    Renderable::~Renderable() {
        if(mesh_) {
            delete mesh_;
        }
        if(material_) {
            delete material_;
        }
    }

}