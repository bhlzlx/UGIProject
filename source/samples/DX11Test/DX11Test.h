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

    class DX11Test : public UGIApplication {
    private:
        void*                           _hwnd;                                             //
        //      
        uint32_t                        _flightIndex;                                      // flight index
        //      
        size_t                          _GPUMemorySize;
        char                            _GPUDescription[128];
        // D3D11 device
        ComPtr<ID3D11Device>            _d3d11Device;
        ComPtr<ID3D11DeviceContext>     _d3d11Context;
        //
        // D3D11.1 device
        ComPtr<ID3D11Device1>           _d3d11Device1;
        ComPtr<ID3D11DeviceContext1>    _d3d11Context1;

        SwapchainDX11*                  _swapchain;

        uint32_t                        _msaaQuality;

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

    public:
        DeviceDX11() {            
        }

    };

    class SwapchainDX11 {
    private:
        uint32_t                    _enabled_11_1;
        union {
            DXGI_SWAP_CHAIN_DESC    _swapchainDesc;
            DXGI_SWAP_CHAIN_DESC1   _swapchainDesc1;
        };
        union {
            IDXGISwapChain*         _swapchain;
            IDXGISwapChain1*        _swapchain1;
        };
    public:
        SwapchainDX11( const DXGI_SWAP_CHAIN_DESC& swapchainDesc )
            : _swapchainDesc( swapchainDesc )
        {
        }

        SwapchainDX11* CreateSwapchain( ComPtr<IDXGIFactory2> factory, DXGI_SWAP_CHAIN_DESC1 );
        SwapchainDX11* CreateSwapchain( ComPtr<IDXGIFactory1> factory, DXGI_SWAP_CHAIN_DESC );

        void onResize( uint32_t width, uint32_t height );
    };

}

UGIApplication* GetApplication();