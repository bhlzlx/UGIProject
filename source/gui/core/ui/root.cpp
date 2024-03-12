#include "root.h"
#include "core/declare.h"
#include "core/display_objects/display_components.h"

namespace gui {

    void Root::createDisplayObject() {
        Component::createDisplayObject();
        reg.emplace_or_replace<dispcomp::is_root>(root_);
        reg.emplace_or_replace<dispcomp::batch_node>(root_);
    }

}