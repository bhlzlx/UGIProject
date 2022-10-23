#include "Renderable.h"
#include <render_components/MeshPrimitive.h>
#include <Pipeline.h>

namespace ugi {

    bool Renderable::prepared() const {
        return mesh_->prepared();
    }

    void Renderable::draw(RenderCommandEncoder* encoder) {
        mesh_->bind(encoder);
        pipeline_->applyMaterial(material_);
        pipeline_->setRasterizationState()
    }

}