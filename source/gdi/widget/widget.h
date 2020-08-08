#pragma once
#include <hgl/math/Vector.h>
#include <hgl/type/RectScope.h>
#include <algorithm>
#include <unordered_map>
#include <set>
#include <vector>
#include <string>

namespace ugi {
    namespace gdi {

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
			Component*          _owner;
            Component*          _collector;
			Group*              _group;
			hgl::RectScope2f    _rect;
			uint32_t            _depth;
			WidgetType          _type;
			std::string			_name;          ///> 这样的可以用Name来代替，省内存
            std::string         _key;           ///> 这样的可以用Name来代替，省内存
            TransformHandle     _transformHandle;
            uint32_t            _colorMask;
            uint32_t            _extraFlags;
        protected:
            bool _registComponentKey( const std::string& key );
		public:
			Widget( Component* owner, WidgetType type = WidgetType::widget)
				: _owner( owner )
				, _group(nullptr)
				, _rect(0, 0, 16, 16)
				, _depth(0)
				, _type(type)
                , _transformHandle{ 0, 0 }
			{
			}

			uint32_t depth() const;
			void setName(const std::string& name) {
				_name = name;
			}
			void setName(std::string&& name) {
				_name = std::move(name);
			}
            
            void setKey( const std::string& key ) {
                if(_registComponentKey(key)) {
                    _key = key;
                }
            }
            void setKey( std::string&& key ) {
                if(_registComponentKey(key)) {
                    _key = std::move(key);
                }
            }

            bool isStatic() {
                // 低16位是uniform索引，如果是0代表永远不改变，是单位矩阵，不可改
                return ( 0 == (_transformHandle.handle & 0xff));
            }

            void setTransform( const Transform& transform );
            void setColorMask( uint32_t colorMask );
            void setGray( float gray );

            const std::string& key() {
                return _key;                
            }
            void setDepth(uint32_t depth);
            WidgetType type() const;
            void setRect(const hgl::RectScope2f& rect);
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