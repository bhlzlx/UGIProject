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
                if( (*iter)->_group != (*groupBegin)->_group ) {
                    if((*groupBegin)->_group) {
                        std::sort(groupBegin, iter, []( Widget* a, Widget* b) {
                            return a->_depth < b->_depth;
                        });
                    }
                    groupBegin = iter;
                }
                ++iter;
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
            widget->_component = this;
            _widgetsRecord.insert(widget);
            _widgets.push_back(widget);
            if( widget->type() == WidgetType::component ) {
                Component* subComponent = (Component*)widget;
                subComponent->_postAddAction();
            }
            _postUpdateAction(); // 更新
            _postSortAction();
        }

        void Component::removeWidget( Widget* widget ) {
            auto iter = _widgets.begin();
            while(iter!=_widgets.end()) {
                if(*iter == widget) {
                    break;
                }
                ++iter;
            }
            if(iter ==_widgets.end()) {
                return; // 这个子控件不存在！
            }
            _widgetsRecord.erase(widget);
            if( widget->type() != WidgetType::component ) {
                _postRemoveAction();
            }
        }

        void Component::collectDrawItems() {
            if(!dirtyFlag(ComponentDirtyFlagBits::Drawing)) {
                return;
            }
            for( auto & drawItem: _drawItems) {
                if( drawItem.type == ComponentDrawItemType::drawData ) {
                    _system->trackDrawData(drawItem.drawData);
                }
            }
            _drawItems.clear();
            // ==
            ComponentDrawItem drawItem;
            IGeometryBuilder* geomBuilder = _system->geometryBuilder();
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
            if(geomBuilder->state() == IGeometryBuilder::GeometryBuildState::building) {
                // 如果之前有收集 draw data，那么需要结束收集
                auto drawData = geomBuilder->endBuild();
                drawItem.type = ComponentDrawItemType::drawData;
                drawItem.drawData = drawData;
                this->_drawItems.emplace_back(drawItem);
            }
            setDirtyFlag(ComponentDirtyFlagBits::Drawing, 0);
        }

        void Component::_postUpdateAction() {
            ComponentDrawDataCollectorHandler::Action action;
            action.type = ComponentDrawDataCollectorHandler::Action::Type::update;
            action.component = this;
            _system->trackCollectionAction(action);
            setDirtyFlag( ComponentDirtyFlagBits::Drawing, 1 );
        }
        void Component::_postAddAction() {
            ComponentDrawDataCollectorHandler::Action action;
            action.type = ComponentDrawDataCollectorHandler::Action::Type::add;
            action.component = this;
            _system->trackCollectionAction(action);
            setDirtyFlag( ComponentDirtyFlagBits::Drawing, 1 );
        }
        
        void Component::_postRemoveAction() {
            ComponentDrawDataCollectorHandler::Action action;
            action.type = ComponentDrawDataCollectorHandler::Action::Type::remove;
            action.component = this;
            _system->trackCollectionAction(action);
            setDirtyFlag( ComponentDirtyFlagBits::Drawing, 1 );
        }

        void Component::_postSortAction() {
            ComponentLayoutHandler::Action action;
            action.component = this;
            action.type = ComponentLayoutHandler::Action::Type::Sort;
            _system->trackLayoutAction(action);
            setDirtyFlag( ComponentDirtyFlagBits::Sort, 1 );
        }

        void Component::setDepth( uint32_t depth ) {
            _depth = depth;
            setDirtyFlag(ComponentDirtyFlagBits::Sort, 1);
        }

        void Component::sortDepth() {
            if(dirtyFlag( ComponentDirtyFlagBits::Sort)) {
                _depthSort();
                setDirtyFlag(ComponentDirtyFlagBits::Sort, 0);
            }
        }

        void Component::setDirtyFlag( ComponentDirtyFlagBits flagBit, uint32_t val ) {
            if(val) {
                _dirtyFlags = _dirtyFlags | (uint32_t)flagBit;
            } else {
                _dirtyFlags = _dirtyFlags & (~(uint32_t)flagBit);
            }
        }

        bool Component::dirtyFlag( ComponentDirtyFlagBits flagBit ) {
            return (bool)(_dirtyFlags & (uint32_t)flagBit);
        }
    }
}