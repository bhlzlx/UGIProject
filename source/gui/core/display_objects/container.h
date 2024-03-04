#pragma once

#include <memory>

#include "../declare.h"
#include "core/display_objects/display_object.h"
#include "display_object.h"
#include <vector>

namespace gui {

    class Container : public DisplayObject {
    private:
        std::vector<DisplayObject*>         children_;
        // render mode
        // camera
        bool                                opaque_;
        Rect<int>                           clipRect_;
    public:
        Container() {
        }

        DisplayObject* addChild(DisplayObject* child);

    };

}