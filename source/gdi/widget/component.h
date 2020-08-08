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
            friend class UI2DSystem;
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
            // 这个数据成员仅在 component 收集子控件绘制信息的时候用
            // 所以我们在调用 设置变换方法的时候，是需要改变它的
            // 如果当前设置的控件正在渲染，那么还需要修改渲染数据
            std::unordered_map<Widget*,Transform>   _transformTable;
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
            Component( Component* owner );

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
                return _owner;
            }

            Component* createComponent();
            ColoredRectangle* createColoredRectangle( uint32_t color);

            /* collect draw items */
            void collectDrawItems();
            void sortDepth();
            //
            const std::vector<Widget*>& widgets() const {
                return _widgets;
            }
            Widget* find( const std::string& key );
            //
            void registTransform( Widget* widget, const Transform& transform );
            Transform* getTransform( Widget* widget );
            // == regist key - widget
            bool registWidget( const std::string& key, Widget* widget );
            bool registWidget( Widget* widget );
            void unregistWidget( Widget* widget );
            // ==
            void syncTransform( Widget* widget, const Transform& transform );
            void syncExtraFlags( Widget* widget, uint32_t colorMask, uint32_t extraFlags );
            // == Get draw items
            const std::vector<ComponentDrawItem>& drawItems() const;
        };


    }
}