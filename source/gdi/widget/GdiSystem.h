#pragma once
#include <vector>
#include <cstdint>

namespace hgl {
    namespace assets {
        class AssetsSource;
    }
}

namespace ugi {
    namespace gdi {

        class Component;
        class IGeometryBuilder;
        class GDIContext;

        class IComponentDrawingManager {
        protected:
        public:
            virtual void onNeedUpdate( Component* component ) = 0;
            virtual void onAddToDisplayList( Component* component ) = 0;
            virtual void onRemoveFromDisplayList( Component* Component ) = 0;
            virtual void onTick() = 0;
            //
        };

        IComponentDrawingManager* CreateComponentDrawingManager( GDIContext* context );

        class UI2DSystem {
        private:
            Component*                  _rootComponent;
            std::vector<Component*>     _components;
            IComponentDrawingManager*   _drawingManager;
            //
            GDIContext*                 _gdiContext;
            hgl::assets::AssetsSource*  _assetsSource;
            IGeometryBuilder*           _geomBuilder;
            uint8_t                     _initialized;
            //
		private:
			void onAddComponent( Component* component ) {
                _drawingManager->onAddToDisplayList(component);
            }
			void onUpdateComponent( Component* component ) {
                _drawingManager->onNeedUpdate(component);
            }
        public:
            UI2DSystem()
                : _rootComponent( nullptr )
                , _components {}
                , _drawingManager( nullptr )
                , _gdiContext( nullptr )
                , _geomBuilder( nullptr )
                , _initialized( 0 )
            {
            }

            bool initialize( GDIContext* context );

            void addComponent( Component* component, uint32_t depth = 0 );

            Component* createComponent();

            IGeometryBuilder* geometryBuilder() const {
                return _geomBuilder;
            }
            //
            void onTick();
        };
        
        extern bool InitializeGdiSystem( GDIContext* context, hgl::assets::AssetsSource* assetsSource );
        extern UI2DSystem* GetGdiSystem();
        extern void DeinitializeGdiSystem();

    }
}