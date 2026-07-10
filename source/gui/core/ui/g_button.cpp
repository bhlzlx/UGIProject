#include "g_button.h"
#include "g_text_field.h"
#include "image.h"
#include "core/declare.h"
#include "utils/byte_buffer.h"

namespace gui {

    GButton::GButton() {
        type_ = ObjectType::Button;
    }

    GButton::~GButton() {
        if (buttonController_) delete buttonController_;
    }

    void GButton::initButtonController() {
        if (buttonController_) return;
        buttonController_ = new Controller();
        buttonController_->parent_ = this;

        // page order: 0=up, 1=down, 2=over, 3=selectedOver, 4=disabled, 5=selectedDisabled
        buttonController_->pageNames_.push_back(UP);
        buttonController_->pageIDs_.push_back("0");
        buttonController_->pageNames_.push_back(DOWN);
        buttonController_->pageIDs_.push_back("1");
        buttonController_->pageNames_.push_back(OVER);
        buttonController_->pageIDs_.push_back("2");
        buttonController_->pageNames_.push_back(SELECTED_OVER);
        buttonController_->pageIDs_.push_back("3");
        buttonController_->pageNames_.push_back(DISABLED);
        buttonController_->pageIDs_.push_back("4");
        buttonController_->pageNames_.push_back(SELECTED_DISABLED);
        buttonController_->pageIDs_.push_back("5");
        buttonController_->selectedIndex_ = 0;

        controllers_.push_back(buttonController_);
    }

    void GButton::constructFromResource() {
        Component::constructFromResource();
        initButtonController();
    }

    void GButton::setupAfterAdd(ByteBuffer& buffer, int startPos) {
        Component::setupAfterAdd(buffer, startPos);
        if (!titleObject_) titleObject_ = getChild("title");
        if (!iconObject_)  iconObject_  = getChild("icon");
    }

    void GButton::onControllerChanged(Controller* ctl) {
        Component::onControllerChanged(ctl);
        if (ctl == relatedController_) {
            setSelected(ctl->selectedPage() == relatedPageId_);
        }
    }

    void GButton::setState(std::string const& val) {
        if (!buttonController_) return;
        if (buttonController_->hasPage(val))
            buttonController_->setSelectedPage(val);
    }

    void GButton::updateState() {
        if (!buttonController_) return;
        if (selected_) {
            if (over_)  setState(SELECTED_OVER);
            else        setState(UP);  // selected state: "up" for selected
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
            if (img) {
                // TODO: set icon from URL
            }
        }
    }

    void GButton::onTouchBegin() {
        down_ = true;
        updateState();
    }

    void GButton::onTouchEnd() {
        if (down_) {
            down_ = false;
            if (changeStateOnClick_) {
                if (mode_ == ButtonMode::Check) {
                    setSelected(!selected_);
                } else if (mode_ == ButtonMode::Radio) {
                    if (!selected_) setSelected(true);
                }
            }
        }
        updateState();
    }

}
