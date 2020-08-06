#include "GdiSystem.h"
#include "component.h"
#include <gdi.h>
#include <geometry/geometryBuilder.h>
#include <geometry/geometryDrawData.h>
#include <ugi/CommandBuffer.h>

namespace ugi {
    namespace gdi {

        bool UI2DSystem::initialize( GDIContext* context ) {
            if( this->_initialized) {
                return true;
            }
            _rootComponent = new Component(this);
			_rootComponent->setName("root");
            _gdiContext = context;
            _assetsSource = context->assetsSource();
            _geomBuilder = CreateGeometryBuilder(_gdiContext);
            _initialized = true;
            return true;
        }

        void UI2DSystem::addComponent( Component* component, uint32_t depth ) {
            component->setDepth(depth);
            _rootComponent->addWidget(component);
            std::sort( _components.begin(), _components.end(), []( Component* a, Component* b)->bool {
                return a->depth() > b->depth();
            });
            // == 更新 drawdata
            ComponentDrawDataCollectorHandler::Action action;
            action.type = ComponentDrawDataCollectorHandler::Action::Type::add;
            action.component = component;
            trackCollectionAction(action);
        }

        Component* UI2DSystem::createComponent() {
            auto component = new Component(this);
            return component;
        }

        ColoredRectangle* UI2DSystem::createColoredRectangle() {
            auto rc = new ColoredRectangle( 0xffffffff );
            return rc;
        }

        void UI2DSystem::prepareResource( ugi::ResourceCommandEncoder* encoder, UniformAllocator* allocator ) {
            _preparedDrawData.clear();
            _preparedDrawItems.clear();
            // 初始化数据
            ComponentDrawItem drawItem;
            drawItem.type = ComponentDrawItemType::component;
            drawItem.component = _rootComponent;
            _preparedDrawItems.push_back(drawItem);
            // 循环访问渲染数据
            while(!_preparedDrawItems.empty()) {
                auto drawItem = _preparedDrawItems.back();
                _preparedDrawItems.pop_back();
                if(drawItem.type == ComponentDrawItemType::drawData) {
                    drawItem.drawData->prepareResource( encoder, allocator);
                    _preparedDrawData.push_back(drawItem.drawData);
                } else if( drawItem.type == ComponentDrawItemType::component ) {
                    Component* component = drawItem.component;
                    for(auto& subDrawItem : component->drawItems()) {
                        _preparedDrawItems.push_back(subDrawItem);
                    }
                }
            }
        }

        void UI2DSystem::draw( ugi::RenderCommandEncoder* encoder ) {
			encoder->bindPipeline(_gdiContext->pipeline());	// bind pipeoline
			// draw!
            for( auto& drawItem : _preparedDrawData ) {
                drawItem->draw(encoder);
            }
        }

        void UI2DSystem::trackDrawData( GeometryDrawData* drawData ) {
            _geomDataDeletor.post(drawData);
        }

        void UI2DSystem::trackCollectionAction( const ComponentDrawDataCollectorHandler::Action& action ) {
            _drawDataCollector.post(action);
        }

        void UI2DSystem::trackLayoutAction( const ComponentLayoutHandler::Action& action ) {
            _layoutMonitor.post(action);
        }

        void UI2DSystem::onTick() {
            _geomDataDeletor.tick(); // 绘制资源Tick
            _layoutMonitor.tick(); // 布局 tick
            _drawDataCollector.tick(); // 收集器Tick
        }

        void ComponentDrawDataCollectorHandler::_collectComponentForUpdate( Component* component ) {
            component->collectDrawItems();
        }

        void ComponentDrawDataCollectorHandler::_collectComponentForAdd( Component* component ) {
            Component* superComponent = component->superComponent();
            superComponent->collectDrawItems();
            component->collectDrawItems();
        }

        void ComponentDrawDataCollectorHandler::_collectComponentForRemove( Component* component ) {
            // 从 super component 里取出来之后，对 super component 的引用就没了，所以这里。。。啥也不用干！
            Component* superComponent = component->superComponent();
            // if( superComponent) {
            //     superComponent->collectDrawItems();
            // }
        }

    }
}