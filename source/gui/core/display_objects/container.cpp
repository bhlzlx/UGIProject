#include "container.h"

namespace gui {

    DisplayObject* Container::addChild(DisplayObject* child) {
        children_.push_back(child);
        return child;
    }

}