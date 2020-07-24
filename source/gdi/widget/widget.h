#pragma once
#include <hgl/math/Vector.h>
#include <hgl/type/RectScope.h>
#include <algorithm>
#include <unordered_map>
#include <set>

namespace ugi {
    namespace gdi {

        enum class LayoutType {
            vbox = 0,
            hbox = 1,
            grid = 2,
        };
        
        enum class WidgetType {
            widget = 0,
            group = 1,
            component = 2,
        };

        class Widget;
        class Component;
        class Group;
        class Layout {
        private:
            LayoutType              _type;
            std::vector<Widget*>    _widgets;
        public:
        };

        class Widget {
            friend class Component;
        protected:
            Component*          _component;
            Group*              _group;
            hgl::RectScope2f    _rect;
            uint32_t            _depth;
        public:
            Widget()
                : _rect(0, 0, 16, 16)
                , _depth(0)
            {
            }

            uint32_t depth() {
                return _depth;
            }
        };

        // Group 是若干个控件的集合，实际上只是比普通widget多了一个自动布局的功能
        class Group : public Widget {
            friend class Component;
        protected:
            std::vector<Widget*>    _widgets;
        public:
            void addChild( Widget* widget );
        };

        class RelativeLayoutManager {
        private:
        public:
        };

       
    }
}