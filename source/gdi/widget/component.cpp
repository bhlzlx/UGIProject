#include "component.h"
#include <geometry/geometryBuilder.h>
#include <widget/widget.h>
#include <widget/GdiSystem.h>

namespace ugi {

    namespace gdi {

        void Component::_depthSort() {
            // 按Group排序
            std::sort( _widgets.begin(), _widgets.end(), []( Widget* a, Widget* b) {
                uint32_t depth1 = a->_group ? a->_group->_depth : a->_depth;
                uint32_t depth2 = b->_group ? b->_group->_depth : b->_depth;
                return depth1 < depth2;
            });
            // 组内排序
            auto iter = _widgets.begin();
            auto groupBegin = iter;
            while(iter!=_widgets.end()) {
                ++iter;
                if( (*iter)->_group != (*groupBegin)->_group ) {
                    if((*groupBegin)->_group) {
                        std::sort(groupBegin, iter, []( Widget* a, Widget* b) {
                            return a->_depth < b->_depth;
                        });
                    }
                    groupBegin = iter;
                }
            }
            // 最后的组
            if( (*groupBegin)->_group && iter != groupBegin ) {
                std::sort(groupBegin, iter, []( Widget* a, Widget* b) {
                    return a->_depth < b->_depth;
                });
            }
        }
        
        void Component::addWidget( Widget* widget ) {
            if( _widgetsRecord.find(widget) != _widgetsRecord.end()) {
                return;
            }
            _widgetsRecord.insert(widget);
        }

        GeometryDrawData* Component::collectDrawItems( UI2DSystem* uisys ) {
            for( auto & drawItem: _drawItems) {
                if( drawItem.type == ComponentDrawItemType::drawData ) {
                    uisys->trackDrawData( drawItem.drawData);
                }
            }
            _drawItems.clear();
            // ==
            ComponentDrawItem drawItem;
            IGeometryBuilder* geomBuilder = uisys->geometryBuilder();
            GeometryDrawData* drawData = nullptr;
            // =========================================================
            for( auto& widget : _widgets) 
            {
                auto type = widget->type();
                switch( type ) 
                {
                    case WidgetType::component: {
                        if(geomBuilder->state() == IGeometryBuilder::GeometryBuildState::building) {
                            // 如果之前有收集 draw data，那么需要结束收集
                            auto drawData = geomBuilder->endBuild();
                            drawItem.type = ComponentDrawItemType::drawData;
                            drawItem.drawData = drawData;
                            this->_drawItems.emplace_back(drawItem);
                        }
                        drawItem.type = ComponentDrawItemType::component;
                        drawItem.component = (Component*)widget;
                        this->_drawItems.emplace_back(drawItem);
                        break;
                    }
                    case WidgetType::rectangle: {
                        if( geomBuilder->state() == IGeometryBuilder::GeometryBuildState::idle ) {
                            geomBuilder->beginBuild();
                        }
                        const auto& rect = widget->rect();
                        ColoredRectangle* coloredRect = (ColoredRectangle*)widget;
                        geomBuilder->drawRect( rect.GetLeft(), rect.GetTop(), rect.GetRight(), rect.GetBottom(), coloredRect->color());
                        break;
                    }
                    // == 以下两种暂时不做处理
                    case WidgetType::group:
                    case WidgetType::widget:
                    default:
                    break;
                }
            }
            drawData = geomBuilder->endBuild();
        }

        
    }
}