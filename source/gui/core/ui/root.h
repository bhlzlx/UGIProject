#pragma  once

#include "core/declare.h"
#include <core/ui/component.h>

namespace gui {

    class Root : public Component {
    private:
    public:
        Root()
         : Component()
        {
            type_ = ObjectType::Root;
        }

        virtual void createDisplayObject() override;
    };

}