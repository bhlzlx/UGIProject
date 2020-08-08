#include "component.h"
#include <geometry/geometryBuilder.h>
#include <geometry/geometryDrawData.h>
#include <widget/widget.h>
#include <widget/GdiSystem.h>
#include <ugi/UGIUtility.h>

namespace ugi {

    namespace gdi {

        Component::Component( Component* owner )
            : Widget( owner, WidgetType::component )
            , _widgets()
            , _groups()
            , _widgetsRecord()
            , _drawItems()
            , _system(nullptr)
            , _dirtyFlags(0)
        {
        }

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
            widget->_collector = this;
            _widgetsRecord.insert(widget);
            _widgets.push_back(widget);
            if( widget->type() == WidgetType::component ) {
                Component* subComponent = (Component*)widget;
                subComponent->_postAddAction();
            }
            if(!registWidget(widget)) {
                // 重复了，刷LOG!
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
            widget->_collector = nullptr;
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
                        auto geomTransHandle = geomBuilder->drawRect( rect.GetLeft(), rect.GetTop(), rect.GetWidth(), rect.GetHeight(), coloredRect->color(), true);
                        auto drawDataIndex = _drawItems.size();
                        coloredRect->_transformHandle.index = (uint32_t)drawDataIndex;
                        coloredRect->_transformHandle.handle = geomTransHandle;
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
            for (auto& drawItem : _drawItems) {
                if (drawItem.type == ComponentDrawItemType::drawData) {
                    drawItem.drawData->setScissor(
                        _scissor.GetLeft(),
                        _scissor.GetRight(),
                        _scissor.GetTop(),
                        _scissor.GetBottom()
                    );
                    // drawData->setTransform()
                }
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

        void Component::setScissor(float left, float top, float width, float height) {
            _scissor.Set(left, top, width, height);
            for (auto& drawItem : _drawItems) {
                if (drawItem.type == ComponentDrawItemType::drawData) {
                    drawItem.drawData->setScissor(left, left + width, top, top + height);
                }
            }
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

        bool Component::registWidget( const std::string& key, Widget* widget ) {
            if(!key.length()) {
                return false; // 这键值是空的！
            }
            UGIHash<APHash> keyHasher;
            keyHasher.hashBuffer(key.c_str(), key.length());
            uint64_t hash = keyHasher;
            auto it = _registTable.find(hash);
            if(it !=_registTable.end()) {
                return false; // 这个新KEY已经有人占用了，目前不可用
            }
            _registTable[hash] = widget;
            if( widget->key().length()) {
                UGIHash<APHash> oldKeyHasher;
                oldKeyHasher.hashBuffer(widget->key().c_str(), widget->key().length());
                uint64_t oldHash = oldKeyHasher;
                auto oldIt = _registTable.find(oldHash);
                if( oldIt != _registTable.end()) {
                    _registTable.erase(oldIt);
                }
            }
            widget->setKey(key);
            return true;
        }

        bool Component::registWidget( Widget* widget ) {
            if( widget->key().length()) {
                UGIHash<APHash> keyHader;
                keyHader.hashBuffer(widget->key().c_str(), widget->key().length());
                uint64_t hash = keyHader;
                auto id = _registTable.find(hash);
                if( id != _registTable.end()) {
                    widget->setKey("#");
                    return false;
                } else {
                    _registTable[hash] = widget;
                    return true;
                }
            }
            return false;
        }

        void Component::unregistWidget( Widget* widget ) {
            UGIHash<APHash> oldKeyHasher;
            oldKeyHasher.hashBuffer(widget->key().c_str(), widget->key().length());
            uint64_t oldHash = oldKeyHasher;
            auto oldIt = _registTable.find(oldHash);
            if( oldIt != _registTable.end()) {
                _registTable.erase(oldIt);
            }
        }

        Widget* Component::find( const std::string& key ) {
            UGIHash<APHash> keyHasher;
            keyHasher.hashBuffer(key.c_str(), key.length());
            uint64_t hash = keyHasher;
            auto it = _registTable.find(hash);
            if(it !=_registTable.end()) {
                return it->second;
            }
            return nullptr;
        }

        ColoredRectangle* Component::createColoredRectangle( uint32_t color ) {
            ColoredRectangle* wgt = new ColoredRectangle( this, color );
            return wgt;
        }

        Component* Component::createComponent() {
            Component* component = new Component(this);
            component->_system = _system;
            return component;
        }

        void Component::registTransform( Widget* widget, const Transform& transform ) {
            _transformTable[widget] = transform;
        }
        
        Transform* Component::getTransform( Widget* widget ) {
            auto it = _transformTable.find(widget);
            if( it == _transformTable.end()) {
                return nullptr;
            }
            return &it->second;
        }

        void Component::syncTransform( Widget* widget, const Transform& transform ) {
            auto it = _widgetsRecord.find(widget);
            if(it != _widgetsRecord.end()) {
                auto handle = widget->_transformHandle;
                auto drawData = _drawItems[handle.index].drawData;
                drawData->setElementTransform( 
                    handle.handle, 
                    transform.anchor, 
                    transform.scale, 
                    transform.radian, 
                    transform.offset
                );
            }
        }

        void Component::syncExtraFlags( Widget* widget, uint32_t colorMask, uint32_t extraFlags ) {
            auto it = _widgetsRecord.find(widget);
            if(it != _widgetsRecord.end()) {
                auto handle = widget->_transformHandle;
                auto drawData = _drawItems[handle.index].drawData;
                drawData->setElementColorMask( 
                    handle.handle,
                    colorMask
                );
                drawData->setElementExtraFlags(
                    handle.handle,
                    extraFlags
                );
            }
        }

        const std::vector<ComponentDrawItem>& Component::drawItems() const {
                return _drawItems;
            }
    }
}