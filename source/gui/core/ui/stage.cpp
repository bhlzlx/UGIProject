#include "stage.h"
#include "root.h"

namespace gui {

    void Stage::initialize() {
        this->ui2dRoot_ = new Root();
    }

}