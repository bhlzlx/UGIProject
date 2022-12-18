#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>

// d3d 11 headers
#include <dxgi.h>
#include <d3dcommon.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <wrl/client.h>

namespace ugi {

    using namespace Microsoft::WRL;

    class SwapchainDX11;
    class DeviceDX11;

    constexpr uint32_t BackbufferCount = 1;
    class DX11Test : public UGIApplication {
    private:
        void*                           _hwnd;                                             //
        //      
        uint32_t                        _flightIndex;                                      // flight index
        DeviceDX11*                     _device;
        SwapchainDX11*                  _swapchain;
        ID3D11RasterizerState*          _rs;
        ID3D11BlendState*               _bs;
        ID3D11DepthStencilState*        _ds;
        //
        ID3D11VertexShader*             vs_;
        ID3D11PixelShader*              ps_;
        ID3D11Buffer*                   vbo_;
        ID3D11InputLayout*              inputLayout_;
        float                           _width;
        float                           _height;
    public:
        virtual bool initialize( void* _wnd,  comm::IArchive* archive);
        virtual void resize( uint32_t _width, uint32_t _height );
        virtual void release();
        virtual void tick();
        virtual const char * title();
        virtual uint32_t rendererType();
        virtual ~DX11Test() {}
    };

    class DeviceDX11 {
    private:
        ID3D11Device*                   _device;
        ID3D11DeviceContext*            _context;
        uint32_t                        _minorVersion;
        size_t                          _graphicsCardMemorySize;
        uint32_t                        _msaaQuality;
        char                            _graphicsCardDescription[128];
        uint32_t                        _bufferCount = BackbufferCount;
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


    /**
     * @brief 
     * 其实这个Swapchain应该写两个版本的，细化一下实现
     */
    class SwapchainDX11 {
        friend class DeviceDX11;
    private:
        DeviceDX11*                 _device;
        HWND                        _hwnd;
        union {
            IDXGISwapChain*         _swapchain;
            IDXGISwapChain1*        _swapchain1;
        };
        ID3D11Texture2D*            _renderBuffers[BackbufferCount];
        ID3D11Texture2D*            _depthStencilBuffer;
        ID3D11RenderTargetView*     _renderTargetViews[BackbufferCount];
        ID3D11DepthStencilView*     _depthStencilView;
        int                         sampleCount_;
        int                         quality_;
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

        void setRenderTarget(ID3D11DeviceContext* context);

        void clear(float(& channels)[4], float depth, uint32_t stencil );

        void present();

        bool onResize( uint32_t width, uint32_t height );
    };

}

UGIApplication* GetApplication();