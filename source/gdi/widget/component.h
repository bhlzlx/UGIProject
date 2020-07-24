#pragma once
#include "widget.h"

namespace ugi {
    namespace gdi {

        class Component : public Widget {
        protected:
            std::vector<Widget*>    _widgets;       // ordered by depth
            std::vector<Group*>     _groups;
            //
            std::set<Widget*>       _widgetsRecord;
            
            //
            void _depthSort();
        public:
            Component(){
            }

            void addWidget( Widget* widget );
            void addGroup( Group* group );

        };

        class ComponentDrawDataManager {
        private:
            std::map< Component*,  >
        };


    }
}