#include "hit_test.h"
#include <utils/byte_buffer.h>
#include <core/ui_objects/object.h>

namespace gui {


    PixelHitTestData::PixelHitTestData() noexcept
        : width(0)
        , height(0)
        , scale(0)
        , pixels(nullptr)
    {
    }

    PixelHitTestData::~PixelHitTestData() {
        if(pixels) {
            delete []pixels;
            pixels = nullptr;
        }
    }

    void PixelHitTestData::load(ByteBuffer<PackageBlocks>* buffer) {
        buffer->skip(4);
        width = buffer->read<int>();
        scale = 1.0f / buffer->read<int8_t>();
        auto byteCount = buffer->read<int>();
        height =  byteCount / width;
        pixels = new uint8_t[byteCount];
        for(uint32_t i = 0; i<byteCount; ++i) {
            pixels[i] = buffer->read<uint8_t>();
        }
    }


    PixelHitTest::PixelHitTest(PixelHitTestData* data, int offsetX, int offsetY)
        : data_(data)
        , offsetX(offsetX)
        , offsetY(offsetY)
        , scaleX(1.f)
        , scaleY(1.f)
    {
    }

    bool PixelHitTest::hitTest(Component* component, glm::vec2 localPoint) const {
        return true;
        // int x = floor((localPoint.x / scaleX - offsetX) * data_->scale);
        // int y = floor(((component->height() - localPoint.y) / scaleY - offsetY) * _data->scale);
        // if (x < 0 || y < 0 || x >= _data->pixelWidth)
        //     return false;

        // ssize_t pos = y * _data->pixelWidth + x;
        // ssize_t pos2 = pos / 8;
        // ssize_t pos3 = pos % 8;

        // if (pos2 >= 0 && pos2 < (ssize_t)_data->pixelsLength)
        //     return ((_data->pixels[pos2] >> pos3) & 0x1) > 0;
        // else
        //     return false;

    }


}