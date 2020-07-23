#pragma once
#include <hgl/math/Vector.h>
#include <hgl/type/RectScope.h>
#include <algorithm>

namespace ugi {
    namespace gdi {

        enum class LayoutType {
            VBoxLayout = 0,
            HBoxLayout = 1,
            GridLayout = 2,
        };

        class Layout {
        private:
            LayoutType              _type;
            std::vector<Widget*>    _widgets;
        public:
        };

        class Widget {
        private:
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
        private:
            std::vector<Widget*>    _widgets;
        public:
            void addChild( Widget* widget );
        };

        class RelativeLayoutManager {
        private:
        public:
        };

        class Component : public Widget {
        protected:
            std::vector<Widget*>    _widgets;
            std::vector<Group*>     _groups;
            //
            
        public:
            Component(){
            }

            void addWidget( Widget* widget );
            void addGroup( Group* group );
            

        };

    }
}