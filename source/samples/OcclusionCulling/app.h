#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>
#include <ugi/ugi_declare.h>
#include <ugi/ugi_types.h>
#include <ugi/descriptor_binder.h>
#include <hgl/math/Vector.h>
// #include <glm/glm.hpp>
#include <OCCapsule.h>

namespace ugi {

    struct CVertex {
        hgl::vec3<float> position;
        hgl::vec3<float> normal;
        hgl::vec2<float> coord;
	};

    class App : public UGIApplication {
    private:
        void*                   _hwnd;                                             //
        ugi::RenderSystem*      _renderSystem;                                     //
        ugi::Device*            _device;                                           //
        ugi::Swapchain*         _swapchain;                                        //
        //
        ugi::Fence*             _frameCompleteFences[MaxFlightCount];              // command buffer 被GPU消化完会给 fence 一个 signal, 用于双缓冲或者多缓冲逻辑隔帧等待
        ugi::Semaphore*         _renderCompleteSemaphores[MaxFlightCount];         // 用于GPU内部等待
        ugi::CommandBuffer*     _commandBuffers[MaxFlightCount];                   // command buffer for each frame
        ugi::CommandQueue*      _graphicsQueue;                                    // graphics queue
        ugi::CommandQueue*      _uploadQueue;                                      // upload queue
        ugi::GraphicsPipeline*          _pipeline;
        ///> ===========================================================================
        ugi::DescriptorBinder*     _argumentGroup;                                    // 
        // ugi::Buffer*            m_uniformBuffer;
        ugi::Buffer*            m_vertexBuffer;                                     //
        ugi::Buffer*            m_indexBuffer;
        ugi::Drawable*          m_drawable;

        ugi::UniformAllocator*  _uniformAllocator;
        res_descriptor_t      m_uniformDescriptor;

		OCCapsule*              _oc;
        //
        uint32_t                _flightIndex;                                      // flight index
        //
        float                   _width;
        float                   _height;
		//
		float 					_offsetX;
		float 					_offsetY;
    public:
        virtual bool initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource );
        virtual void resize( uint32_t _width, uint32_t _height );
        virtual void release();
        virtual void tick();
        virtual const char * title();
        virtual uint32_t rendererType();

		virtual void onKeyEvent(unsigned char _key, eKeyEvent _event) override;
		virtual void onMouseEvent(eMouseButton _bt, eMouseEvent _event, int _x, int _y) override;
	};

}

UGIApplication* GetApplication();