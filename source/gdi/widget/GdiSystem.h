#pragma once
#include <vector>
#include <array>
#include <cstdint>
#include <widget/component.h>

namespace hgl {
    namespace assets {
        class AssetsSource;
    }
}

namespace ugi {

    class RenderCommandEncoder;
    class ResourceCommandEncoder;
    class UniformAllocator;
    //
    namespace gdi {

        template< class T, class D, uint32_t FrameCount>
        class FrameDeferredHandler {
        private:
            std::array< std::vector<T>,FrameCount >     _queueArray;
            uint32_t                                    _frameIndex;
        public:
            FrameDeferredHandler()
                : _queueArray()
                , _frameIndex(0) 
            {
            }

            void tick() {
                D handler;
                for( T& res : _queueArray[_frameIndex]) {
                    handler(res);
                }
                _queueArray[_frameIndex].clear();
                //
                ++_frameIndex;
                _frameIndex = _frameIndex % FrameCount;
            }

            template< class N = T >
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

        struct GeometryDestroyer {
            void operator ()( GeometryDrawData* data );
        };

        class ComponentDrawDataCollectorHandler {
        public:
            struct Action {
                enum class Type {
                    add,
                    update,
                    remove
                };
                Type        type;
                Component*  component;
            };
        private:
            void _collectComponentForUpdate( Component* component );
            void _collectComponentForAdd( Component* component );
            void _collectComponentForRemove( Component* component );
        public:
            void operator()( const Action& action ) {
                switch(action.type) {
                    case Action::Type::add: {
                        _collectComponentForAdd(action.component);
                        break;
                    }
                    case Action::Type::update: {
                        _collectComponentForUpdate(action.component);
                        break;
                    }
                    case Action::Type::remove: {
                        _collectComponentForRemove(action.component);
                        break;
                    }
                }
            }
        };

        class ComponentLayoutHandler {
        public:
            struct Action {
                enum class Type {
                    Sort = 0
                };
                Type type;
                Component* component;
            };
        public:
            void operator()( const Action& action ) {
                switch( action.type) {
                    case Action::Type::Sort: {
                        action.component->sortDepth();
                        break;
                    }
                }
            }
        };

        class UI2DSystem {
            typedef FrameDeferredHandler<GeometryDrawData*,GeometryDestroyer,2> GeomDrawDataDeferredDeletor;
            typedef FrameDeferredHandler<ComponentDrawDataCollectorHandler::Action,ComponentDrawDataCollectorHandler,2> ComponentDrawDataCollector;
            typedef FrameDeferredHandler<ComponentLayoutHandler::Action,ComponentLayoutHandler,2> ComponentLayoutMonitor;
        private:
            Component*                              _component;     ///> root compoennt
            hgl::Vector2f                           _windowSize;
            //          
            GDIContext*                             _gdiContext;
            hgl::assets::AssetsSource*              _assetsSource;
            IGeometryBuilder*                       _geomBuilder;
            uint8_t                                 _initialized;
            
            /*扔到销毁队列的draw data( 如果有必要，可以隔帧销毁 )*/
            std::vector<GeometryDrawData*>          _trackedDrawData;
            GeomDrawDataDeferredDeletor             _geomDataDeletor;
            ComponentDrawDataCollector              _drawDataCollector;
            ComponentLayoutMonitor                  _layoutMonitor;

            std::vector<ComponentDrawItem>          _preparedDrawItems;
            std::vector<GeometryDrawData*>          _preparedDrawData;
        private:
        public:
            UI2DSystem()
                : _component( nullptr )
                , _gdiContext( nullptr )
                , _assetsSource( nullptr )
                , _geomBuilder( nullptr )
                , _initialized( 0 )
            {
            }

            bool initialize( GDIContext* context );
            // == Add Component
            void addComponent( Component* component, uint32_t depth = 0 );
            // == Render
            void prepareResource( ugi::ResourceCommandEncoder* encoder, UniformAllocator* allocator );
            void draw( ugi::RenderCommandEncoder* encoder );
            // == Tick
            void onTick();
            void onResize( uint32_t width, uint32_t height );
            // == Track Resource & Drawing & Updating
            void trackDrawData( GeometryDrawData* drawData );
            void trackCollectionAction( const ComponentDrawDataCollectorHandler::Action& action );
            void trackLayoutAction( const ComponentLayoutHandler::Action& action );
            // == Getters
            IGeometryBuilder* geometryBuilder() const {
                return _geomBuilder;
            }
            Component* root() const {
                return _component;
            }
        };

    }
}