#include "object.h"
#include "core/declare.h"
#include "core/display_objects/display_components.h"
#include "utils/byte_buffer.h"
#include <core/gui_context.h>
#include <core/ui/component.h>

namespace gui {

    void Object::release() {
        delete this;
    }

    Object::~Object() {}


    void Object::onInit() {

    }
    void Object::onSizeChanged() {

    }
    void Object::onScaleChanged() {

    }
    void Object::onGrayedChanged() {

    }
    void Object::onPositionChanged() {

    }
    void Object::onAlphaChanged() {

    }
    void Object::onVisibleChanged() {

    }
    void Object::onEnter() {

    }
    void Object::onExit() {

    }
    void Object::onControllerChanged(Controller* controller) {

    }

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
            setVisible(true);
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

    void Object::setupAfterAdd(ByteBuffer& buffer, int startPos) {
    }

    void Object::internalSetParent(Component* comp) {
        parent_ = comp;
    }

    void Object::handleControllerChanged(Controller* controller) {
        handlingController_ = true; {
        }
        handlingController_ = false;
    }

    void Object::createDisplayObject() {}

    void Object::setVisible(bool val) {
        visible_ = val;
        if(val) {
            reg.emplace_or_replace<dispcomp::visible>(dispobj_);
        } else {
            reg.remove<dispcomp::visible>(dispobj_);
        }
        reg.emplace_or_replace<dispcomp::visible_changed>(dispobj_);
    }

    void Object::setPixelSnapping(bool val) {
    }

    bool Object::inContainer() const {
        return dispobj_ && dispobj_.parent();
    }

    void Object::removeFromParent() {
        if(parent_) {
            parent_->removeChild(this);
        }
    }

    void Object::setPosition( glm::vec3 const& val) {
        position_ = val;
        dispobj_.setPosition(val);
    }

    void Object::setSize(Size2D<float> const& size) {
        dispobj_.setSize(size);
    }

    void Object::setPivot(glm::vec2 const& pivot, bool asAnchor) {
        pivot_ = pivot;
        pivotAsAnchor_ = asAnchor;
        dispobj_.setPivot(pivot);
    }

    void Object::setScale(float x, float y) {
        scale_ = {x, y};
        dispobj_.setScale({x, y});
    }

    void Object::setSkew(float x, float y) {
        skew_ = {x, y};
        dispobj_.setSkew(skew_);
    }

    void Object::setAlpha(float val) {
        alpha_ = val;
        // dispobj_.
    }

    void Object::setGrayed(bool val) {
        grayed_ = val;
    }

}
