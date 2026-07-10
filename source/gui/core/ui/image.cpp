#include "image.h"
#include "render/render_data.h"
#include <core/data_types/ui_types.h>
#include <core/declare.h>
#include <core/display_objects/display_components.h>
#include <utils/byte_buffer.h>

namespace gui {

    void Image::constructFromResource() {
        auto contentItem = this->packageItem_->getBranch();
        sourceSize_ = {(float)contentItem->width_, (float)contentItem->height_};
        contentItem = contentItem->getHighSolution();
        contentItem->load();
        NTexture* tex = contentItem->texture_;
        auto const& uvRc = tex->uvRc();
        auto& image_desc_t = reg.get_or_emplace<dispcomp::image_desc_t>(dispobj_);
        image_desc_t.textureBlock.uv[0] = uvRc.base;
        image_desc_t.textureBlock.uv[1] = { uvRc.right(), uvRc.bottom() };
        image_desc_t.textureBlock.size.x = contentItem->width_;
        image_desc_t.textureBlock.size.y = contentItem->height_;
        image_desc_t.grid9 = contentItem->scale9Grid_;
        image_desc_t.scaleByTile = contentItem->scaledByTile_;
        //
        auto &graphics = reg.get_or_emplace<dispcomp::item_render_data>(dispobj_);
        graphics.texture = contentItem->texture_->handle();
        //
        setSize(sourceSize_);
    }

    void Image::setSize(Size2D<float> const& size) {
        Object::setSize(size);
        reg.emplace_or_replace<dispcomp::mesh_dirty>(dispobj_);
        reg.emplace_or_replace<dispcomp::batch_dirty>(dispobj_);
    }

    void Image::setupBeforeAdd(ByteBuffer& buffer, int startPos) {
        Object::setupBeforeAdd(buffer, startPos);
        buffer.seekToBlock(startPos, ObjectBlocks::FillInfo);
        auto& image_desc_t = reg.get_or_emplace<dispcomp::image_desc_t>(dispobj_);
        if(buffer.read<bool>()) {
            image_desc_t.ext.color = buffer.read<uint32_t>();
        }
        image_desc_t.ext.flip = buffer.read<FlipType>();
        image_desc_t.ext.fill = buffer.read<FillMethod>();
        if(image_desc_t.ext.fill != FillMethod::None) {
            image_desc_t.ext.fillOrig = buffer.read<FillOrigin>();
            image_desc_t.ext.fillClockwise = buffer.read<bool>();
            image_desc_t.ext.fillAmount = buffer.read<float>();
        }
    }

    void Image::setupAfterAdd(ByteBuffer& buffer, int startPos) {
        Object::setupAfterAdd(buffer, startPos);
        // nothing...
        return;
    }

    void Image::createDisplayObject() {
        Object::createDisplayObject();
        reg.emplace_or_replace<dispcomp::mesh_dirty>(dispobj_);
        reg.emplace_or_replace<dispcomp::image_desc_t>(dispobj_);
        auto& gfx = reg.get_or_emplace<dispcomp::item_render_data>(dispobj_);
        gfx.args.color = glm::vec4(1.f, 1.f, 1.f, alpha_);
    }

    void Image::setColor(Color4B val) {
        color_ = val;
        if (dispobj_) {
            auto& gfx = reg.get<dispcomp::item_render_data>(dispobj_);
            gfx.args.color = glm::vec4(val.r / 255.f, val.g / 255.f, val.b / 255.f, val.a / 255.f);
            auto& s = reg.get_or_emplace<dispcomp::args_need_sync>(dispobj_);
            s.mask |= dispcomp::Asm_Color;
        }
    }

}
