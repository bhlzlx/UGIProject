#include "g_button.h"
#include "g_text_field.h"
#include "image.h"
#include "core/declare.h"
#include "utils/byte_buffer.h"

namespace gui {

    GButton::GButton() {
        type_ = ObjectType::Button;
    }

    GButton::~GButton() = default;

    void GButton::constructFromResource() {
        Component::constructFromResource();
        // 按钮的 controller 在 constructFromResource 里已从 .fui 加载，
        // constructExtension 会找到它并注册事件
    }

    void GButton::constructExtension(ByteBuffer& buff) {
        // Extension block (from Component::constructFromResource line 170)
        buff.seekToBlock(0, ComponentBlocks::Ext);

        mode_ = (ButtonMode)buff.read<uint8_t>();
        buff.read<csref>();  // sound url, skip
        buff.read<float>();  // soundVolumeScale
        int downEffect = buff.read<uint8_t>();
        float downEffectValue = buff.read<float>();
        if (downEffect == 2) {
            setPivot({0.5f, 0.5f}, pivotAsAnchor_);
        }

        // 从已有的 controllers 中找到 button controller
        buttonController_ = getController("button");

        titleObject_ = getChild("title");
        iconObject_  = getChild("icon");
        if (titleObject_) {
            auto* txt = dynamic_cast<GTextField*>(titleObject_);
            if (txt) title_ = txt->text();
        }

        if (mode_ == ButtonMode::Common)
            setState(UP);

        // 注册事件监听 (与 C# 对齐)
        addEventListener("onTouchBegin", [this](EventContext*) { onTouchBegin(); });
        addEventListener("onClick",      [this](EventContext*) { onTouchEnd(); });
        addEventListener("onRollOver",   [this](EventContext*) { over_ = true;  updateState(); });
        addEventListener("onRollOut",    [this](EventContext*) { over_ = false; updateState(); });
    }

    void GButton::setupAfterAdd(ByteBuffer& buffer, int startPos) {
        Component::setupAfterAdd(buffer, startPos);
        if (!titleObject_) titleObject_ = getChild("title");
        if (!iconObject_)  iconObject_  = getChild("icon");

        buffer.seekToBlock(startPos, ComponentBlocks::Ext);
        relatedController_ = getControllerAt(buffer.read<int16_t>());
        relatedPageId_     = buffer.read<csref>();
    }

    void GButton::onControllerChanged(Controller* ctl) {
        Component::onControllerChanged(ctl);
        if (ctl == relatedController_) {
            setSelected(ctl->selectedPage() == relatedPageId_);
        }
    }

    void GButton::setState(std::string const& val) {
        if (buttonController_ && buttonController_->hasPage(val))
            buttonController_->setSelectedPage(val);
    }

    void GButton::updateState() {
        if (selected_) {
            setState(over_ ? SELECTED_OVER : UP);
        } else {
            if (down_)  setState(DOWN);
            else if (over_) setState(OVER);
            else        setState(UP);
        }
    }

    void GButton::setSelected(bool val) {
        if (mode_ == ButtonMode::Common) return;
        if (selected_ == val) return;
        selected_ = val;
        updateState();
        if (relatedController_ && !relatedPageId_.empty()
            && relatedController_->selectedPage() != relatedPageId_) {
            if (val)
                relatedController_->setSelectedPage(relatedPageId_);
            else if (mode_ == ButtonMode::Radio && relatedController_->selectedPage() == relatedPageId_)
                relatedController_->setSelectedIndex(0);
        }
    }

    void GButton::setMode(ButtonMode val) { mode_ = val; }

    void GButton::setTitle(std::string const& val) {
        title_ = val;
        if (titleObject_) {
            auto* txt = dynamic_cast<GTextField*>(titleObject_);
            if (txt)
                txt->setText(selected_ && !selectedTitle_.empty() ? selectedTitle_ : title_);
        }
    }

    void GButton::setIcon(std::string const& val) {
        icon_ = val;
        if (iconObject_) {
            auto* img = dynamic_cast<Image*>(iconObject_);
            if (img) { /* TODO: set icon from URL */ }
        }
    }

    void GButton::onTouchBegin() {
        down_ = true;
        updateState();
    }

    void GButton::onTouchEnd() {
        if (down_) {
            down_ = false;
            if (mode_ == ButtonMode::Check) {
                setSelected(!selected_);
            } else if (mode_ == ButtonMode::Radio) {
                if (!selected_) setSelected(true);
            }
        }
        updateState();
    }

}
