#pragma once
#include <core/declare.h>

namespace gui {

    class GUIContext {
    private:
        ObjectDestroyManager* destroyManager_;
        ObjectTable* objectTable_;
    public:
        ObjectDestroyManager* destroyManager();
        ObjectTable* objectTable();
    };

}