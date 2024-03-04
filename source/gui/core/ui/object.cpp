#include "object.h"
#include "core/data_types/transition.h"
#include "core/declare.h"
#include "core/display_objects/display_object.h"
#include "utils/byte_buffer.h"
#include <cassert>
#include <core/gui_context.h>

namespace gui {

    void Object::release() {
        delete this;
    }

    Object::~Object() {}

    void Object::setupBeforeAdd(ByteBuffer& bufRef, int startPos) {
        auto buffer = &bufRef;
        buffer->seekToBlock(startPos, ObjectBlocks::Props);
        buffer->skip(5);
        id_ = buffer->read<std::string>();
        name_ = buffer->read<std::string>();
        float f1, f2, f3, f4;
        f1 = buffer->read<int>();
        f2 = buffer->read<int>();
        if(buffer->version >= 7) {
            f3 = buffer->read<int>();
        } else {
            f3 = 0;
        }
        setPosition({f1, f2, f3}); // set position
        if(buffer->read<bool>()) {
            rawSize_.width = buffer->read<int>();
            rawSize_.height = buffer->read<int>();
            setSize(rawSize_);
        }
        if(buffer->read<bool>()) {//
            minSize_.width = buffer->read<int>();
            maxSize_.width = buffer->read<int>();
            minSize_.height = buffer->read<int>();
            maxSize_.height = buffer->read<int>();
        }
        if(buffer->read<bool>()) {
            f1 = buffer->read<float>();
            f2 = buffer->read<float>();
            setScale(f1, f2);
        }
        if(buffer->read<bool>()) {
            f1 = buffer->read<float>();
            f2 = buffer->read<float>();
            setSkew(f1, f2);
        }
        if(buffer->read<bool>()) {
            f1 = buffer->read<float>();
            f2 = buffer->read<float>();
            setPivot({f1, f2}, buffer->read<bool>());
        }
        f1 = buffer->read<float>();
        if(f1 != 1) {
           setAlpha(f1);
        }
        f1 = buffer->read<float>();
        if(f1 != 1) {
            setRotation(f1);
        }
        if(!buffer->read<bool>()) {
            setVisible(false);
        }
        if(!buffer->read<bool>()) {
            setTouchable(false);
        }
        if(buffer->read<bool>()) {
            setGrayed(true);
        }
        uint8_t blendMode = buffer->read<uint8_t>(); // blend mode
        int filter = buffer->read<uint8_t>();
        if(filter == 1) {
            // read color filter data
            buffer->read<float>();
            buffer->read<float>();
            buffer->read<float>();
            buffer->read<float>();
        }
        data_ = buffer->read<std::string>(); // user data???
    }

    void Object::internalSetParent(Component* comp) {
        parent_ = comp;
    }

    void Object::handleControllerChanged(Controller* controller) {
        handlingController_ = true; {
        }
        handlingController_ = false;
    }

    void Object::createDisplayObject() {
        dispobj_ = DisplayObject::createDisplayObject();
    }

}
