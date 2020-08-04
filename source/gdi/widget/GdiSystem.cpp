#include "GdiSystem.h"
#include "component.h"
#include "../geometry/geometryBuilder.h"
#include "../gdi.h"

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

            virtual void onTick() override {

            }
		};

        IComponentDrawingManager* CreateComponentDrawingManager( UI2DSystem* sys ) {
            auto drawingManager = new ComponentDrawingManager(sys);
            return drawingManager;
        }


        UI2DSystem* __GdiSystem = nullptr;

        bool InitializeGdiSystem( GDIContext* context) {
            if(!__GdiSystem) {
                if(!context) {
                    return false;
                }
                __GdiSystem = new UI2DSystem();
                __GdiSystem->initialize( context );
            }
            return true;
        }

        void DeinitializeGdiSystem() {

        }

        UI2DSystem* GetGdiSystem() {
            return __GdiSystem;
        }

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

        void UI2DSystem::trackDrawData( GeometryDrawData* drawData ) {
            _geomDataDeletor.post(drawData);
        }

    }
}