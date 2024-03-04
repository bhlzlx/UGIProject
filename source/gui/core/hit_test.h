#pragma once
#include "utils/byte_buffer.h"
#include <core/declare.h>

namespace gui {

    class IHitTest {
    public:
        virtual bool hitTest(Component* component, glm::vec2 localPoint) const { // why obj but not reference?
            return true;
        } 
    };

    struct PixelHitTestData {
        int         width;
        int         height;
        float       scale;
        uint8_t*    pixels;
        //
        void load(ByteBuffer& buffer);
        PixelHitTestData() noexcept;
        ~PixelHitTestData();
    };

    class PixelHitTest : public IHitTest {
    private:
        PixelHitTestData*       data_;
    public:
        int                     offsetX;
        int                     offsetY;
        float                   scaleX;
        float                   scaleY;
        //
        PixelHitTest(PixelHitTestData* data, int offsetX, int offsetY);
        virtual bool hitTest(Component* component, glm::vec2 localPoint) const override;
    };

}