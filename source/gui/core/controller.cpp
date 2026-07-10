#include "controller.h"
#include <core/data_types/value.h>
#include <core/ui/component.h>
#include <algorithm>

namespace gui {

    uint32_t Controller::nextPageID_ = 0;

    // ============= ControllerAction =============

    void ControllerAction::run(Controller* ctl, std::string const& prevPage, std::string const& curPage) {
        bool fromOk = fromPages.empty()
            || std::find(fromPages.begin(), fromPages.end(), prevPage) != fromPages.end();
        bool toOk = toPages.empty()
            || std::find(toPages.begin(), toPages.end(), curPage) != toPages.end();
        if (fromOk && toOk)
            enter(ctl);
        else
            leave(ctl);
    }

    void ControllerAction::setup(ByteBuffer& buffer) {
        int cnt = buffer.read<int16_t>();
        fromPages.resize(cnt);
        for (int i = 0; i < cnt; ++i)
            fromPages[i] = buffer.read<csref>();

        cnt = buffer.read<int16_t>();
        toPages.resize(cnt);
        for (int i = 0; i < cnt; ++i)
            toPages[i] = buffer.read<csref>();
    }

    ControllerAction* ControllerAction::create(ControllerActionType type) {
        switch (type) {
        case ControllerActionType::ChangePage:   return new ChangePageAction();
        case ControllerActionType::PlayTransition: return new PlayTransitionAction();
        default: return nullptr;
        }
    }

    // ============= ChangePageAction =============

    void ChangePageAction::setup(ByteBuffer& buffer) {
        ControllerAction::setup(buffer);
        objectId       = buffer.read<csref>();
        controllerName = buffer.read<csref>();
        targetPage     = buffer.read<csref>();
    }

    void ChangePageAction::enter(Controller* ctl) {
        if (!ctl || !ctl->parent()) return;
        auto* target = ctl->parent()->getChildByID(objectId);
        if (!target) return;
        auto* comp = dynamic_cast<Component*>(target);
        auto* cc = comp ? comp->getController(controllerName) : nullptr;
        if (cc) cc->setSelectedPage(targetPage);
    }

    void ChangePageAction::leave(Controller* ctl) {
        // no-op for now
    }

    // ============= PlayTransitionAction =============

    void PlayTransitionAction::setup(ByteBuffer& buffer) {
        ControllerAction::setup(buffer);
        transitionName = buffer.read<csref>();
        playTimes      = buffer.read<int>();
        delay          = buffer.read<float>();
        stopOnExit     = buffer.read<bool>();
    }

    void PlayTransitionAction::enter(Controller* ctl) {
        if (!ctl || !ctl->parent()) return;
        Transition* trans = ctl->parent()->getTransition(transitionName);
        if (trans) {
            trans->play(playTimes, delay);
        }
    }

    void PlayTransitionAction::leave(Controller* ctl) {
        if (!stopOnExit || !ctl || !ctl->parent()) return;
        Transition* trans = ctl->parent()->getTransition(transitionName);
        if (trans) trans->stop();
    }

    // ============= Controller =============

    Controller::Controller()
        : selectedIndex_(-1), prevIndex_(-1) {}

    Controller::~Controller() {
        for (auto* a : actions_) delete a;
    }

    void Controller::setSelectedIndex(int index) {
        if (selectedIndex_ == index) return;
        if (index >= (int)pageIDs_.size()) return;

        changing_ = true;
        prevIndex_ = selectedIndex_;
        selectedIndex_ = index;
        if (parent_) parent_->applyController(this);
        dispatchEvent(Value("onChanged"), nullptr, Value());
        changing_ = false;
    }

    void Controller::setSelectedPage(std::string const& name) {
        auto it = std::find(pageNames_.begin(), pageNames_.end(), name);
        if (it != pageNames_.end())
            setSelectedIndex((int)(it - pageNames_.begin()));
        else if (!pageNames_.empty())
            setSelectedIndex(0);
    }

    std::string Controller::selectedPage() const {
        if (selectedIndex_ < 0 || selectedIndex_ >= (int)pageNames_.size())
            return "";
        return pageNames_[selectedIndex_];
    }

    bool Controller::hasPage(std::string const& name) const {
        return std::find(pageNames_.begin(), pageNames_.end(), name) != pageNames_.end();
    }

    void Controller::runActions() {
        if (prevIndex_ < 0 || selectedIndex_ < 0) return;
        std::string prev = (prevIndex_ < (int)pageIDs_.size()) ? pageIDs_[prevIndex_] : "";
        std::string cur  = (selectedIndex_ < (int)pageIDs_.size()) ? pageIDs_[selectedIndex_] : "";
        for (auto* action : actions_)
            action->run(this, prev, cur);
    }

    void Controller::setup(ByteBuffer& buffer) {
        enum class CtlBlock : uint8_t { Props = 0, Pages = 1, Actions = 2 };
        int beginPos = buffer.pos();

        buffer.seekToBlock(beginPos, CtlBlock::Props);
        name_                  = buffer.read<csref>();
        autoRadioGroupDepth_   = buffer.read<bool>();

        buffer.seekToBlock(beginPos, CtlBlock::Pages);
        int cnt = buffer.read<int16_t>();
        pageIDs_.reserve(cnt);
        pageNames_.reserve(cnt);
        for (int i = 0; i < cnt; ++i) {
            pageIDs_.push_back(buffer.read<csref>());
            pageNames_.push_back(buffer.read<csref>());
        }

        int homePageIndex = 0;
        if (buffer.version >= 2) {
            int homePageType = buffer.read<uint8_t>();
            switch (homePageType) {
            case 1:
                homePageIndex = buffer.read<int16_t>();
                break;
            case 2:
            case 3: {
                auto varName = buffer.read<csref>();
                auto it = std::find(pageNames_.begin(), pageNames_.end(), varName);
                if (it != pageNames_.end())
                    homePageIndex = (int)(it - pageNames_.begin());
                else
                    homePageIndex = 0;
                break;
            }
            default: break;
            }
        }

        buffer.seekToBlock(beginPos, CtlBlock::Actions);
        cnt = buffer.read<int16_t>();
        for (int i = 0; i < cnt; ++i) {
            int nextPos = buffer.read<uint16_t>();
            nextPos += buffer.pos();
            auto type = (ControllerActionType)buffer.read<uint8_t>();
            auto* action = ControllerAction::create(type);
            if (action) {
                action->setup(buffer);
                actions_.push_back(action);
            }
            buffer.setPos(nextPos);
        }

        if (parent_ && !pageIDs_.empty())
            selectedIndex_ = homePageIndex;
        else
            selectedIndex_ = -1;
    }

}
