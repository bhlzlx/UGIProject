#pragma once
#include "widget.h"
#include <map>

namespace ugi {
    namespace gdi {

        class UI2DSystem;
        class GeometryDrawData;
        //
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

        enum class ComponentDirtyFlagBits {
            Drawing     = (1<<0),
            Sort        = (1<<1),
        };

        struct WidgetTransform {
            hgl::Vector2f anchor;
            hgl::Vector2f scale;
            hgl::Vector2f offset;
        };

        // component 还需要一个子控件管理器，比如说 ID,动画,变换

        class Component : public Widget {
        protected:
            std::vector<Widget*>                    _widgets;       // ordered by depth
            std::vector<Group*>                     _groups;
            hgl::RectScope2f                        _scissor;
            //  
            std::set<Widget*>                       _widgetsRecord; //
            //  
            std::vector<ComponentDrawItem>          _drawItems;
            UI2DSystem*                             _system;
            //  
            uint32_t                                _dirtyFlags;
            //  名字哈希和控件的映射
            std::unordered_map<uint64_t,Widget*>    _registTable;
        protected:            
            //
            void _depthSort();
            // == 更新
            void _postUpdateAction();
            void _postAddAction();
            void _postRemoveAction();
            // == 
            void _postSortAction();
        public:
            Component( UI2DSystem* system )
                : Widget(WidgetType::component )
                , _widgets()
                , _groups()
                , _widgetsRecord()
                , _drawItems()
                , _system(system)
                , _dirtyFlags(0)
            {
            }

            void addWidget( Widget* widget );
            void removeWidget( Widget* widget );
            void addGroup( Group* group );
            void setDepth( uint32_t depth );
            void setScissor( float left, float top, float width, float height );

            void setDirtyFlag( ComponentDirtyFlagBits flagBit, uint32_t val );
            bool dirtyFlag( ComponentDirtyFlagBits flagBit );

            UI2DSystem* system() {
                return _system;
            }

            Component* superComponent() const {
                return _component;
            }
            /* collect draw items */
            void collectDrawItems();
            void sortDepth();
            //
            Widget* find( const std::string& key );
            //
            // == regist key - widget
            bool registWidget( const std::string& key, Widget* widget );
            bool registWidget( Widget* widget );
            void unregistWidget( Widget* widget );
            // ==
            const std::vector<ComponentDrawItem>& drawItems() const {
                return _drawItems;
            }
        };


    }
}