#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>

// d3d 11 headers
#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <wrl/client.h>

namespace ugi {

    using namespace Microsoft::WRL;

    class SwapchainDX11;
    class DeviceDX11;

    class DX11Test : public UGIApplication {
    private:
        void*                           _hwnd;                                             //
        //      
        uint32_t                        _flightIndex;                                      // flight index
        DeviceDX11*                     _device;
        SwapchainDX11*                  _swapchain;
        float                           _width;
        float                           _height;
    public:
        virtual bool initialize( void* _wnd,  hgl::assets::AssetsSource* assetsSource );
        virtual void resize( uint32_t _width, uint32_t _height );
        virtual void release();
        virtual void tick();
        virtual const char * title();
        virtual uint32_t rendererType();
    };

    class DeviceDX11 {
    private:
        ID3D11Device*                   _device;
        ID3D11DeviceContext*            _context;
        uint32_t                        _minorVersion;
        size_t                          _graphicsCardMemorySize;
        uint32_t                        _msaaQuality;
        char                            _graphicsCardDescription[128];
        uint32_t                        _bufferCount = 2;
    public:
        DeviceDX11() {
        }

        bool intialize();

        IDXGISwapChain1* createSwapchain1( HWND hwnd, uint32_t width, uint32_t height, bool fullscreen = false );
        IDXGISwapChain* createSwapchain( HWND hwnd, uint32_t width, uint32_t height, bool fullscreen = false );

        ID3D11Texture2D* createTexture2D( const D3D11_TEXTURE2D_DESC& desc ) const;
        ID3D11RenderTargetView* createRenderTargetView( ID3D11Texture2D* texture );
        ID3D11DepthStencilView* createDepthStencilView( ID3D11Texture2D* texture );

        // 返回低版本号，用来判断是 DX11 还是 DX11.1
        uint32_t minorVersion() const {
            return _minorVersion;
        }

        ID3D11DeviceContext* context() {
            return _context;
        }

        ID3D11Device* device() {
            return _device;
        }

        ID3D11Device1* queryDevice1();
        ID3D11DeviceContext1* queryDeviceContext1();


    };

    constexpr uint32_t BackbufferNum = 1;

    class SwapchainDX11 {
    private:
        DeviceDX11*                 _device;
        HWND                        _hwnd;
        union {
            IDXGISwapChain*         _swapchain;
            IDXGISwapChain1*        _swapchain1;
        };
        ID3D11Texture2D*            _renderBuffers[BackbufferNum];
        ID3D11Texture2D*            _depthStencilBuffer;
        ID3D11RenderTargetView*     _renderTargetViews[BackbufferNum];
        ID3D11DepthStencilView*     _depthStencilView;
    public:
        SwapchainDX11( DeviceDX11* device, HWND hwnd )
            : _device(device)
            , _hwnd(hwnd)
            , _swapchain( nullptr )
            , _renderBuffers {}
            , _depthStencilBuffer( nullptr )
            , _renderTargetViews {}
            , _depthStencilView(nullptr)
        {
        }

        bool ready() {
            return !!_swapchain;
        }

        void clear( float(& channels)[4], float depth, uint32_t stencil );

        void present();

        bool onResize( uint32_t width, uint32_t height );
    };

}

UGIApplication* GetApplication();