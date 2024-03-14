#include "image.h"
#include "core/display_objects/display_object.h"
#include "render/render_data.h"
#include <core/data_types/ui_types.h>
#include <core/declare.h>
#include <core/display_objects/display_components.h>
#include <utils/byte_buffer.h>

namespace gui {

    void Image::constructFromResource() {
        auto contentItem = this->packageItem_->getBranch();
        rawSize_ = {(float)contentItem->width_, (float)contentItem->height_};
        contentItem = contentItem->getHighSolution();
        contentItem->load();
        auto& image_mesh = reg.get_or_emplace<dispcomp::image_mesh>(dispobj_);
        image_mesh.grid9 = contentItem->scale9Grid_;
        image_mesh.scaleByTile = contentItem->scaledByTile_;
        //
        auto &graphics = reg.get_or_emplace<NGraphics>(dispobj_);
        graphics.texture = contentItem->texture_;
        //
        setSize(rawSize_);
    }

    void Image::setupBeforeAdd(ByteBuffer& buffer, int startPos) {
        Object::setupBeforeAdd(buffer, startPos);
        buffer.seekToBlock(startPos, ObjectBlocks::FillInfo);
        auto& image_mesh = reg.get_or_emplace<dispcomp::image_mesh>(dispobj_);
        if(buffer.read<bool>()) {
            image_mesh.ext.color = buffer.read<uint32_t>();
        }
        image_mesh.ext.flip = buffer.read<FlipType>();
        image_mesh.ext.fill = buffer.read<FillMethod>();
        if(image_mesh.ext.fill != FillMethod::None) {
            image_mesh.ext.fillOrig = buffer.read<FillOrigin>();
            image_mesh.ext.fillClockwise = buffer.read<bool>();
            image_mesh.ext.fillAmount = buffer.read<float>();
        }
    }

    void Image::setupAfterAdd(ByteBuffer& buffer, int startPos) {
        Object::setupAfterAdd(buffer, startPos);
        // nothing...
        return;
    }

    void Image::createDisplayObject() {
        // Object::createDisplayObject();
        dispobj_ = DisplayObject::createDisplayObject();
        reg.emplace_or_replace<dispcomp::mesh_dirty>(dispobj_);
        reg.emplace_or_replace<dispcomp::image_mesh>(dispobj_);
    }

}
