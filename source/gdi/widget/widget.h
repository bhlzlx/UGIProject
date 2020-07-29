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
            rectange = 1,
            group = 2,
            component = 3,
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
            WidgetType          _type;
        public:
            Widget( WidgetType type = WidgetType::widget ) 
                : _component( nullptr )
                , _group( nullptr )
                , _rect(0, 0, 16, 16)
                , _depth(0)
                , _type( type )

            {
            }

            uint32_t depth() const  {
                return _depth;
            }

            void setDepth( uint32_t depth )  {
                _depth = depth;
            }

            WidgetType type() const  {
                return _type;
            }

            void setRect( const hgl::RectScope2f& rect )  {
                _rect = rect;
            }
        };

        class ColoredRectangle : public Widget {
        private:
            uint32_t _color;
        public:
            ColoredRectangle( uint32_t color )
                : Widget( WidgetType::rectange )
                , _color( color )
            {
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