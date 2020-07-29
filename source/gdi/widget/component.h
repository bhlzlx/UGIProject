#pragma once
#include "widget.h"
#include "../geometry/geometryDrawData.h"
#include "../gdi.h"
#include <map>

namespace ugi {
    namespace gdi {

        struct ComponentFlags {
            uint32_t dirty : 1;
        };

        enum class ComponentDrawItemType {
            component   = 0,
            drawData    = 1,
        };
        struct ComponentDrawItem {
            ComponentDrawItemType type;
            union {
                struct {
                    GeometryDrawData*   drawData;
                };
                struct {
                    Component*          component;
                };
            };
        };

        class Component : public Widget {
        protected:
            std::vector<Widget*>                _widgets;       // ordered by depth
            std::vector<Group*>                 _groups;
            //
            std::set<Widget*>                   _widgetsRecord; //
            //
            std::vector<ComponentDrawItem>      _drawItems;
        protected:            
            //
            void _depthSort();
        public:
            Component()
                : _widgets()
                , _groups()
                , _widgetsRecord()
                , _drawItems()
            {
            }

            void addWidget( Widget* widget );
            void addGroup( Group* group );
            void setDepth( uint32_t depth );
            // == 收集绘制内容
            void collectDrawItems();
        };


    }
}0