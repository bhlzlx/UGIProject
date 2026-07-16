#include "object.h"
#include "core/declare.h"
#include "core/display_objects/display_components.h"
#include "utils/byte_buffer.h"
#include <core/gui_context.h>
#include <core/controller.h>
#include <core/ui/component.h>

namespace gui {

    void Object::release() {
        delete this;
    }

    Object::~Object() {}


    void Object::onInit() {

    }
    void Object::onSizeChanged() {
        dispatchEvent(UIEvent::SizeChanged, nullptr, Value());
    }
    void Object::onScaleChanged() {

    }
    void Object::onGrayedChanged() {

    }
    void Object::onPositionChanged() {
        dispatchEvent(UIEvent::PositionChanged, nullptr, Value());
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
        id_ = buffer->read<csref>();
        name_ = buffer->read<csref>();
        float f1, f2, f3;
        f1 = buffer->read<int>();
        f2 = buffer->read<int>();
        f3 = 0;
        setPosition({f1, f2, f3}); // set position
        if(buffer->read<bool>()) {
            sourceSize_.width = buffer->read<int>();
            sourceSize_.height = buffer->read<int>();
            setSize(sourceSize_);
        }
        // 记录初始大小 (= FairyGUI initWidth/initHeight)
        initSize_ = size_;

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
        auto dat = buffer->read<csref>();
        data_ = Value(std::move(dat));

    }

    void Object::setupAfterAdd(ByteBuffer& buffer, int startPos) {
        // Block 1 (Extra): tooltips + group
        buffer.seekToBlock(startPos, ObjectBlocks::Extra);
        auto tips = buffer.read<csref>();
        if (!tips.empty()) tooltips_ = tips;
        int groupId = buffer.read<int16_t>();
        if (groupId >= 0 && parent_) {
            group_ = (Group*)parent_->getChildAt(groupId);
        }

        // Block 3 (Gears): gearCount(short) → [nextPos(ushort) + type(byte) + data]...
        buffer.seekToBlock(startPos, ObjectBlocks::Gears);
        int gearCnt = buffer.read<int16_t>();
        for (int i = 0; i < gearCnt; ++i) {
            ByteBuffer gearData = buffer.readBufferBlock();
            int gearType = gearData.read<uint8_t>();
            if (gearType >= 0 && gearType < Object::kGearCount) {
                auto* gear = createGear(gearType, this);
                if (gear) {
                    gear->setup(gearData);
                    gears_[gearType] = gear;
                }
            }
        }
    }

    void Object::internalSetParent(Component* comp) {
        parent_ = comp;
    }

    void Object::handleControllerChanged(Controller* controller) {
        handlingController_ = true;
        for (int i = 0; i < kGearCount; ++i) {
            if (gears_[i] && gears_[i]->controller() == controller)
                gears_[i]->apply();
        }
        handlingController_ = false;
    }

    void Object::updateGear(int index) {
        if (index >= 0 && index < kGearCount && gears_[index])
            gears_[index]->updateState();
    }

    void Object::checkGearDisplay() {
        if (handlingController_) return;
        for (int i = 0; i < kGearCount; ++i) {
            if (gears_[i] && gears_[i]->controller() && gears_[i]->controller()->changing_)
                gears_[i]->apply();
        }
    }

    void Object::createDisplayObject() {
        dispobj_ = DisplayObject::createDisplayObject();
        // // 初始化渲染参数默认值
        // auto& gfx = reg.get_or_emplace<dispcomp::item_render_data>(dispobj_);
        // gfx.args.color = glm::vec4(1.f, 1.f, 1.f, alpha_);
    }

    void Object::setVisible(bool val) {
        visible_ = val;
        if(val) {
            reg.emplace_or_replace<dispcomp::visible>(dispobj_);
        } else {
            reg.remove<dispcomp::visible>(dispobj_);
        }
        reg.emplace_or_replace<dispcomp::visible_dirty>(dispobj_);
    }

    void Object::setPixelSnapping(bool val) {
        pixelSnapping_ = val;
    }

    bool Object::pixelSnapping() const {
        return pixelSnapping_;
    }

    bool Object::inContainer() const {
        return dispobj_ && dispobj_.parent();
    }

    void Object::removeFromParent() {
        if(parent_) {
            parent_->removeChild(this);
        }
    }

    bool Object::bubbleEvent(std::string const& type, void* data) {
        EventContext ctx(type, this, data);

        // 收集父链 (this → root)
        std::vector<Object*> chain;
        Object* cur = this;
        while (cur) {
            chain.push_back(cur);
            cur = cur->parent();
        }

        // Capture: root → target
        for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
            if (ctx.isStopped()) return true;
            auto& bridge = (*it)->getBridge(type);
            if (!bridge.isEmpty()) bridge.fire(&ctx, true);
        }

        // Target & Bubble: target → root
        for (auto* obj : chain) {
            if (ctx.isStopped()) return true;
            auto& bridge = obj->getBridge(type);
            if (!bridge.isEmpty()) bridge.fire(&ctx, false);
        }
        return true;
    }

    // ---- Position ----

    float Object::xMin() const {
        if (pivotAsAnchor_)
            return position_.x - pivot_.x * size_.width;
        else
            return position_.x;
    }

    float Object::yMin() const {
        if (pivotAsAnchor_)
            return position_.y - pivot_.y * size_.height;
        else
            return position_.y;
    }

    void Object::setXMin(float val) {
        float offset = (pivotAsAnchor_ ? pivot_.x : 0.0f) * size_.width;
        position_.x = val + offset;
        dispobj_.setPosition({position_.x, position_.y});
    }

    void Object::setYMin(float val) {
        float offset = (pivotAsAnchor_ ? pivot_.y : 0.0f) * size_.height;
        position_.y = val + offset;
        dispobj_.setPosition({position_.x, position_.y});
    }

    void Object::setX(float v) {
        setPosition({v, position_.y, position_.z});
    }

    void Object::setY(float v) {
        setPosition({position_.x, v, position_.z});
    }

    void Object::setPosition(glm::vec3 const& val) {
        position_ = val;
        dispobj_.setPosition({val.x, val.y});
        onPositionChanged();
    }

    // ---- Size ----

    void Object::setSizeDirectly(Size2D<float> const& sz) {
        rawSize_ = sz;    // _rawWidth/_rawHeight — 未约束的请求值
        size_ = sz;       // _width/_height — 约束后的实际值 (当前无 min/max 约束)
        dispobj_.setSize({sz.width, sz.height});
    }

    void Object::setSize(Size2D<float> const& sz) {
        float dw = sz.width - size_.width;
        float dh = sz.height - size_.height;
        if (dw == 0 && dh == 0) return;
        setSizeDirectly(sz);
        relations_.onOwnerSizeChanged(dw, dh, false);
        onSizeChanged();
    }

    void Object::setWidth(float val) {
        setSize({val, rawSize_.height});
    }

    void Object::setHeight(float val) {
        setSize({rawSize_.width, val});
    }

    // ---- Pivot / Scale / Skew ----

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
        if (dispobj_ && reg.any_of<dispcomp::item_render_data>(dispobj_)) {
            reg.get<dispcomp::item_render_data>(dispobj_).args.color.a = val;
            auto& s = reg.get_or_emplace<dispcomp::args_need_sync>(dispobj_);
            s.mask |= dispcomp::Asm_Color;
        }
    }

    void Object::setGrayed(bool val) {
        grayed_ = val;
        if (dispobj_ && reg.any_of<dispcomp::item_render_data>(dispobj_)) {
            reg.get<dispcomp::item_render_data>(dispobj_).args.props.x = val ? 1.0f : 0.0f;
            auto& s = reg.get_or_emplace<dispcomp::args_need_sync>(dispobj_);
            s.mask |= dispcomp::Asm_Props;
        }
    }

    bool Object::grayed() const {
        return grayed_;
    }

    void Object::center(bool restraint) {
        if (parent_) {
            setPosition({
                (parent_->width() - size_.width) * 0.5f,
                (parent_->height() - size_.height) * 0.5f,
                position_.z
            });
        }
    }

    void Object::makeFullScreen() {
        if (parent_) {
            setSize({parent_->width(), parent_->height()});
            setPosition({0, 0, position_.z});
        }
    }

    bool Object::checkGearController(GearType gearType, Controller* controller) const {
        if(gears_[gearType] && gears_[gearType]->controller() == controller) {
            return true;
        }
        return false;
    }

    uint32_t Object::addDisplayLock() {
        auto displayGearType = GearType(GearIndex::Display);
        auto* gearDisplay = gears_[displayGearType] ? dynamic_cast<GearDisplay*>(gears_[displayGearType]) : nullptr;
        if (gearDisplay && gearDisplay->controller() != nullptr) {
            uint32_t ret = gearDisplay->addLock();
            checkGearDisplay();
            return ret;
        }
        return 0;
    }

    void Object::releaseDisplayLock(uint32_t token) {
        auto* gearDisplay = gears_[0] ? dynamic_cast<GearDisplay*>(gears_[0]) : nullptr;
        if (gearDisplay && gearDisplay->controller() != nullptr) {
            gearDisplay->releaseLock(token);
            checkGearDisplay();
        }
    }

    void Object::invalidateBatchingState() {
        // TODO: 接入渲染合批系统
    }

    GearBase* Object::getGear(int index) {
        GearBase* gear = gears_[index];
        if (gear == nullptr) {
            gear = createGear(index, this);
            gears_[index] = gear;
        }
        return gear;
    }

}
