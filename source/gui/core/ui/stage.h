#pragma once

#include "utils/singleton.h"
#include <core/ui/root.h>
#include <set>

namespace gui {

    class Stage : public comm::Singleton<Stage> {
    private:
        Root*                   ui2dRoot_;
        std::set<Root*>         ui3dRoots_;
        //
        Stage()
            : ui2dRoot_(nullptr)
            , ui3dRoots_{}
        {
        }
    public:
        void initialize();

        Root* defaultRoot() const {
            return ui2dRoot_;
        }

    };

}