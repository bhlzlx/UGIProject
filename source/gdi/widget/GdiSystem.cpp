#include "GdiSystem.h"
#include "component.h"
#include <gdi.h>
#include <geometry/geometryBuilder.h>
#include <geometry/geometryDrawData.h>
#include <ugi/CommandBuffer.h>

namespace ugi {
    namespace gdi {

		class ComponentDrawingManager : public IComponentDrawingManager {
		private:
            struct DrawData {
                Component*          component;
                GeometryDrawData*   geomDrawData;
            };
            UI2DSystem*                                         _2dsystem;
            Component*                                          _rootComponent;
            std::set<Component*>                                _updateCollection; ///> 需要更新的列表
            /* @brief : 渲染的时候用它来遍历draw data */
            std::vector<GeometryDrawData*>                      _drawDatas;
        private:
		public:
            ComponentDrawingManager( UI2DSystem* dsys )
                : _2dsystem( dsys )
                , _rootComponent( nullptr )
                , _updateCollection {}
                , _drawDatas {}
            {
            }
            /*  一个 component 更新，那么它的父 component 不需要更新，只是更新它自己就行了
            */
            /* todo...
                一个 component 可能对应多个 draw data，component有若干个子component时会有这种情况发生，
                因为子component会打断父component收集，还没处理，有时间处理下
            */
            virtual void onNeedUpdate( Component* component ) override {
                // Component* superComponent = component->superComponent(); // 其实应该用不到父节奏
                component->collectDrawItems(_2dsystem);
            }
            /* 如果一个 component 添加到了父 component 里，那么父 component 则需要重新收集，兄弟component不需要重新收集
            */
            virtual void onAddToDisplayList( Component* component ) override {
                Component* superComponent = component->superComponent();
                superComponent->collectDrawItems(_2dsystem);
                component->collectDrawItems(_2dsystem);
            }
            /*如果一个component被移除了，父控件重新不重新收集都无所谓的
            */
            virtual void onRemoveFromDisplayList( Component* component ) override {
                Component* superComponent = component->superComponent();
                if( superComponent) {
                    superComponent->collectDrawItems(_2dsystem);
                }
            }
		};

        IComponentDrawingManager* CreateComponentDrawingManager( UI2DSystem* sys ) {
            auto drawingManager = new ComponentDrawingManager(sys);
            return drawingManager;
        }


        // UI2DSystem* __GdiSystem = nullptr;

        bool UI2DSystem::initialize( GDIContext* context ) {
            if( this->_initialized) {
                return true;
            }
            _rootComponent = new Component();
            _drawingManager = new ComponentDrawingManager(this);
            _gdiContext = context;
            _assetsSource = context->assetsSource();
            _geomBuilder = CreateGeometryBuilder(_gdiContext);
            _initialized = true;
            return true;
        }

        void UI2DSystem::addComponent( Component* component, uint32_t depth ) {
            component->setDepth(depth);
            std::sort( _components.begin(), _components.end(), []( Component* a, Component* b)->bool {
                return a->depth() > b->depth();
            });
        }

        void UI2DSystem::prepareResource( ugi::ResourceCommandEncoder* encoder, UniformAllocator* allocator ) {
            _preparedDrawData.clear();
            _preparedDrawItems.clear();
            // 初始化数据
            for( auto component : this->_components) {
                ComponentDrawItem drawItem;
                drawItem.type = ComponentDrawItemType::component;
                drawItem.component = component;
                _preparedDrawItems.push_back(drawItem);
            }
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
            for( auto& drawItem : _preparedDrawData ) {
                drawItem->draw(encoder);
            }
        }

        void UI2DSystem::trackDrawData( GeometryDrawData* drawData ) {
            _geomDataDeletor.post(drawData);
        }

        void UI2DSystem::onTick() {
            _geomDataDeletor.tick(); // 绘制资源Tick
        }

    }
}