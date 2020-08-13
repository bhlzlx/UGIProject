#pragma once
#include <hgl/math/Vector.h>
#include <hgl/type/RectScope.h>
#include <algorithm>
#include <unordered_map>
#include <set>
#include <vector>
#include <string>
#include "name.h"

namespace ugi {
    namespace gdi {

        extern NamePool GdiNamePool;

        enum class LayoutType {
            vbox = 0,
            hbox = 1,
            grid = 2,
        };

        enum class WidgetType {
            widget      = 0,
            rectangle   = 1,
            group       = 2,
            component   = 3,
        };

        struct Transform {
            hgl::Vector2f   anchor;
            hgl::Vector2f   scale;
            float           radian;
            hgl::Vector2f   offset;
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

        // 注意，目前Widget还不是带虚函数的，Component这些子类还不是虚继承，以后看看如果有必要就加虚继承


		class Widget {
            struct alignas(4) TransformHandle {
                alignas(4) uint32_t index;
                alignas(4) uint32_t handle;
            };
			friend class Component;
		protected:
			WidgetType          _type;
            Name                _name;
            Name                _key;
			uint32_t            _depth;
            // _owner 是指创建此控件的对象，并不是指控件树结构
			Component*          _owner;
            // _collector 指此控件挂载到哪个 component 上了
            Component*          _collector;
			Group*              _group;
			hgl::RectScope2f    _rect;
            TransformHandle     _transformHandle;
            uint32_t            _colorMask;
            uint32_t            _extraFlags;
        protected:
            bool _registComponentKey( const std::string& key );
		public:
			Widget( Component* owner, WidgetType type = WidgetType::widget);

			uint32_t                depth() const;
			void                    setName(const std::string& name);
			void                    setName(std::string&& name);
            void                    setKey( const std::string& key );
            void                    setKey( std::string&& key );
            bool                    isStatic();
            void                    setTransform( const Transform& transform );
            void                    setColorMask( uint32_t colorMask );
            void                    setGray( float gray );
            const std::string&      key();
            void                    setDepth(uint32_t depth);
            WidgetType              type() const;
            void                    setRect(const hgl::RectScope2f& rect);
            const hgl::RectScope2f& rect();
        };

        class ColoredRectangle : public Widget {
        private:
            uint32_t _color;
        public:
            ColoredRectangle( Component* component, uint32_t color)
                : Widget( component, WidgetType::rectangle )
                , _color(color)
            {
            }
            uint32_t color() const {
                return _color;
            }
            void setColor( uint32_t colorMask ) {
                _color = colorMask;
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
        
    }
}