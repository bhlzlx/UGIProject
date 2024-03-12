#include "image.h"
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
        //
        setSize(rawSize_);
    }

    void Image::setupBeforeAdd(ByteBuffer& buffer, int startPos) {
        Object::setupBeforeAdd(buffer, startPos);
        buffer.seekToBlock(startPos, ObjectBlocks::FillInfo);
        auto& image_info = reg.get_or_emplace<dispcomp::image_info>(dispobj_);
        if(buffer.read<bool>()) {
            image_info.color = buffer.read<uint32_t>();
        }
        image_info.flip = buffer.read<FlipType>();
        image_info.fill = buffer.read<FillMethod>();
        if(image_info.fill != FillMethod::None) {
            image_info.fillOrig = buffer.read<FillOrigin>();
            image_info.fillClockwise = buffer.read<bool>();
            image_info.fillAmount = buffer.read<float>();
        }
    }

    void Image::setupAfterAdd(ByteBuffer& buffer, int startPos) {
        Object::setupAfterAdd(buffer, startPos);
        // nothing...
        return;
    }

}
