#pragma once

#include "core/declare.h"
#include "core/events/event_dispatcher.h"
#include "utils/byte_buffer.h"
namespace gui {

    class ControllerAction {
    public:
        ControllerAction() {}
    };

    class Controller : public EventDispatcher {
        friend class Component;
    private:
        std::string             name_;
        Component*              parent_;
        bool                    autoRadioGroupDepth_;
        bool                    changing_;
        int                     selectedIndex_;
        int                     prevIndex_;
        std::vector<std::string> pageIDs_;
        std::vector<std::string> pageNames_;
        std::vector<ControllerAction> actions_;
        // eventlistener 貌似不用实现

        static uint32_t nextPageID_;
    public:
        Controller()
            : selectedIndex_(-1)
            , prevIndex_(-1)
        {
        }

        void setup(ByteBuffer const& buffer);

        void runActions();

        Component* parent() const {
            return parent_;
        }

        void dispose()
        {
            clearEventListeners();
        }

        int selectedIndex() const {
            return selectedIndex_;
        }

        void setSelectedIndex(int index);

    };

}