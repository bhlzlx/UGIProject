#pragma once

#include "core/data_types/ui_types.h"
#include <vector>

namespace gui {

    class Object;

    class Children {
    private:
        ChildrenRenderOrder     renderOrder_;
        std::vector<Object*>    items_;
    public:
        void add(Object* obj);
        void addAt(Object* obj, uint32_t idx);

        void remove(Object* obj);
        void removeAt(uint32_t idx);

    };

}