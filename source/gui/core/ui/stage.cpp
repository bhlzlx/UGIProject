#include "stage.h"
#include "core/declare.h"
#include "root.h"
#include "object_factory.h"

namespace gui {

    void Stage::initialize() {
        this->ui2dRoot_ = (Root*)ObjectFactory::CreateObject(ObjectType::Root);
    }

}