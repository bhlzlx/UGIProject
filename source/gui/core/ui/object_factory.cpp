#include "object_factory.h"
#include "core/declare.h"
#include "core/package_item.h"
#include "core/ui_objects/component.h"
#include "image.h"
#include <winnt.h>

namespace gui {

    Object* ObjectFactory::CreateObject(PackageItem* item) {
        Object* obj = nullptr;
        switch(item->objType_) {
        case ObjectType::Image: {
            Image* image = new Image();
            obj = image;
            break;
        }
        case ObjectType::MovieClip:
        case ObjectType::Swf:
        case ObjectType::Graph:
        case ObjectType::Loader:
        case ObjectType::Group:
        case ObjectType::Text:
        case ObjectType::RichText:
        case ObjectType::InputText:
        case ObjectType::Component: {
            obj = createComponent(item);
            break;
        }
        case ObjectType::List:
        case ObjectType::Label:
        case ObjectType::Button:
        case ObjectType::ComboBox:
        case ObjectType::ProgressBar:
        case ObjectType::Slider:
        case ObjectType::ScrollBar:
        case ObjectType::Tree:
        case ObjectType::Loader3D:
          break;
        }
        if(obj) {
            obj->packageItem_ = item;
        }
        return obj;
    }

    Object* ObjectFactory::createComponent(PackageItem* item) {
        switch(item->objType_) {
            case ObjectType::Component: {
                return new Component();
            }
            default: {
                return nullptr;
            }
        }
        return nullptr;
    }

}