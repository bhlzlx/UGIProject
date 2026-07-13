#pragma once

#include "core/declare.h"
#include "core/events/event_dispatcher.h"
#include "utils/byte_buffer.h"

namespace gui {

    enum class ControllerActionType : uint8_t {
        PlayTransition,
        ChangePage
    };

    class ControllerAction {
    public:
        std::vector<std::string> fromPages;
        std::vector<std::string> toPages;

        virtual ~ControllerAction() = default;

        void run(Controller* ctl, std::string const& prevPage, std::string const& curPage);
        virtual void enter(Controller* ctl) {}
        virtual void leave(Controller* ctl) {}

        virtual void setup(ByteBuffer& buffer);

        static ControllerAction* create(ControllerActionType type);
    };

    class ChangePageAction : public ControllerAction {
    public:
        std::string objectId;
        std::string controllerName;
        std::string targetPage;
        void setup(ByteBuffer& buffer) override;
        void enter(Controller* ctl) override;
        void leave(Controller* ctl) override;
    };

    class PlayTransitionAction : public ControllerAction {
    public:
        std::string transitionName;
        int         playTimes = 1;
        float       delay = 0;
        bool        stopOnExit = false;
        void setup(ByteBuffer& buffer) override;
        void enter(Controller* ctl) override;
        void leave(Controller* ctl) override;
    };

    class Controller : public EventDispatcher {
        friend class Component;
        friend class GButton;
        friend class Object;
    private:
        std::string                  name_;
        Component*                   parent_ = nullptr;
        bool                         autoRadioGroupDepth_ = false;
        bool                         changing_ = false;
        int                          selectedIndex_ = -1;
        int                          prevIndex_ = -1;
        std::vector<std::string>     pageIDs_;
        std::vector<std::string>     pageNames_;
        std::vector<ControllerAction*> actions_;

        static uint32_t nextPageID_;

    public:
        Controller();
        ~Controller();

        void setup(ByteBuffer& buffer);
        void runActions();

        std::string const& name() const { return name_; }
        Component* parent() const { return parent_; }
        int selectedIndex() const { return selectedIndex_; }
        int previousIndex() const { return prevIndex_; }

        void setSelectedIndex(int index);
        void setSelectedPage(std::string const& name);
        std::string selectedPage() const;

        int pageCount() const { return (int)pageIDs_.size(); }
        std::string const& pageName(int index) const { return pageNames_[index]; }
        bool hasPage(std::string const& name) const;

        void dispose() { clearEventListeners(); }
    };

}
