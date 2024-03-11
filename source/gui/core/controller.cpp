#include "controller.h"
#include <core/data_types/value.h>
#include <core/ui/component.h>

namespace gui {

    void Controller::setSelectedIndex(int index) {
        if(selectedIndex_ == index) {
            return;
        }
        if(index >= pageIDs_.size()) {
            return;
        }
        changing_ = true;
        prevIndex_ = selectedIndex_;
        selectedIndex_ = index;
        parent_->applyController(this);
        dispatchEvent(Value("onChanged"), nullptr, Value());
        changing_ = false;
    }

    void Controller::runActions() {
        return; 
    }

    void Controller::setup(ByteBuffer const& buffer) {
        return;
    }

}