#include "Renderable.h"
#include <render_components/MeshPrimitive.h>
#include <Pipeline.h>
#include <CommandBuffer.h>
#include <VulkanFunctionDeclare.h>

namespace ugi {

    bool Renderable::prepared() const {
        return mesh_->prepared();
    }

    void Renderable::draw(RenderCommandEncoder* encoder) {
        mesh_->bind(encoder);
        pipeline_->applyMaterial(material_);
        pipeline_->setRasterizationState(rasterState_);
        pipeline_->bind(encoder);
        // vkCmdDrawIndexed(*encoder->commandBuffer(), mesh_->indexCount(), 1, 0, )
    }

}