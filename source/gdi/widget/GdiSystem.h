#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include <UniformBuffer.h>
#include <widget/component.h>

namespace hgl {
    namespace assets {
        class AssetsSource;
    }
}

namespace ugi {

    class RenderCommandEncoder;
    class ResourceCommandEncoder;
    //
    namespace gdi {

        template< class T, class D, uint32_t FrameCount>
        class FrameDeferredDeleter {
        private:
            std::array< std::vector<T>,FrameCount >      _queueArray;
            uint32_t                                    _frameIndex;
        public:
            FrameDeferredDeleter()
                : _queueArray()
                , _frameIndex(0) 
            {
            }

            void tick() {
                D deletor;
                for( T& res : _queueArray[_frameIndex]) {
                    deletor(res);
                }
                _queueArray[_frameIndex].clear();
                //
                ++_frameIndex;
                _frameIndex = _frameIndex % FrameCount;
            }

            template< class N >
            void post( N&& res ) {
                uint32_t queueIndex = _frameIndex + FrameCount - 1;
                queueIndex = queueIndex % FrameCount;
                _queueArray[queueIndex].push_back(std::forward<N>(res));
            }
        };

        class Component;
        class IGeometryBuilder;
        class GDIContext;
        class GeometryDrawData;

        class IComponentDrawingManager {
        protected:
        public:
            virtual void onNeedUpdate( Component* component ) = 0;
            virtual void onAddToDisplayList( Component* component ) = 0;
            virtual void onRemoveFromDisplayList( Component* Component ) = 0;
            // virtual void onTick() = 0;
            //
        };

        IComponentDrawingManager* CreateComponentDrawingManager( GDIContext* context );

        struct GeometryDestroyer {
            void operator ()( GeometryDrawData* data ) {
                delete data;
            }
        };

        class UI2DSystem {
            typedef FrameDeferredDeleter<GeometryDrawData*,GeometryDestroyer,2> GeomDrawDataDeferredDeletor;
        private:
            Component*                              _rootComponent;
            std::vector<Component*>                 _components;
            IComponentDrawingManager*               _drawingManager;
            //          
            GDIContext*                             _gdiContext;
            hgl::assets::AssetsSource*              _assetsSource;
            IGeometryBuilder*                       _geomBuilder;
            uint8_t                                 _initialized;
            
            /*扔到销毁队列的draw data( 如果有必要，可以隔帧销毁 )*/
            std::vector<GeometryDrawData*>          _trackedDrawData;
            GeomDrawDataDeferredDeletor             _geomDataDeletor;

            std::vector<ComponentDrawItem>          _preparedDrawItems;
            std::vector<GeometryDrawData*>          _preparedDrawData;
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

            void prepareResource( ugi::ResourceCommandEncoder* encoder, UniformAllocator* allocator );
            void draw( ugi::RenderCommandEncoder* encoder );
            //
            void onTick();
            //
            void trackDrawData( GeometryDrawData* drawData );
        };

    }
}