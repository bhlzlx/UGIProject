#pragma once

#include "core/declare.h"
#include "core/package_item.h"
namespace gui {

    class ObjectFactory {
    private:
    public:
        static Object* CreateObject(PackageItem* item);
        static Object* CreateObject(ObjectType type);
        static Object* createComponent(PackageItem* item);
    };

}