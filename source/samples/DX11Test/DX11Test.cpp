#include "DX11Test.h"
#include <cassert>
#include <hgl/assets/AssetsSource.h>
#include <cmath>

namespace ugi {

    bool DX11Test::initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource ) {
        uint32_t screenWidth = 640, screenHeight = 480;
#ifndef NDEBUG
        uint32_t createDeviceFlags = D3D11_CREATE_DEVICE_DEBUG;
#else
        uint32_t createDeviceFlags = 0;
#endif
        D3D_DRIVER_TYPE driverType[] = {
            D3D_DRIVER_TYPE_HARDWARE,
            D3D_DRIVER_TYPE_WARP,
            D3D_DRIVER_TYPE_REFERENCE,
        };
        const uint32_t numDriverTypes = sizeof(driverType)/sizeof(driverType[0]);
        D3D_FEATURE_LEVEL featureLevels[] = {
            D3D_FEATURE_LEVEL_11_1,
            D3D_FEATURE_LEVEL_11_0,
        };
        const uint32_t numFeatureLevels = sizeof(featureLevels)/sizeof(featureLevels[0]);
        // select a valide feature level & driver type
        D3D_DRIVER_TYPE selectDriverType;
        D3D_FEATURE_LEVEL selectFeatureLevel;
        HRESULT hr;
        for( uint32_t driverTypeIndex = 0; driverTypeIndex<numDriverTypes; ++driverTypeIndex ) {
            selectDriverType = driverType[driverTypeIndex];
            hr = D3D11CreateDevice(
                nullptr, selectDriverType, nullptr, createDeviceFlags, featureLevels,
                numFeatureLevels, D3D11_SDK_VERSION, 
                _d3d11Device.GetAddressOf(), &selectFeatureLevel, _d3d11Context.GetAddressOf()
            );
            if( hr == E_INVALIDARG) { // 不支持当前版本, 尝试低版本
                hr = D3D11CreateDevice(
                    nullptr, selectDriverType, nullptr, createDeviceFlags, &featureLevels[1], numFeatureLevels - 1,
                    D3D11_SDK_VERSION, _d3d11Device.GetAddressOf(), &selectFeatureLevel, _d3d11Context.GetAddressOf()
                );
            }
            if(SUCCEEDED(hr)) {
                break;
            }
        }
        if(FAILED(hr)) {
            MessageBox( NULL, L"Create D3D11 Device Failed!", 0, 0);
            return false;
        }
        // 确认支持的特性
        if ( selectFeatureLevel != D3D_FEATURE_LEVEL_11_0 && selectFeatureLevel != D3D_FEATURE_LEVEL_11_1) {
            MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
            return false;
        }
        _d3d11Device->CheckMultisampleQualityLevels( DXGI_FORMAT_R8G8B8A8_UNORM, 4, &_msaaQuality );
        assert( _msaaQuality && "这个是一定会支持的！");
        //

        ComPtr<IDXGIDevice> dxgiDevice = nullptr;
        ComPtr<IDXGIAdapter> dxgiAdapter = nullptr;
        ComPtr<IDXGIFactory1> dxgiFactory1 = nullptr;
        ComPtr<IDXGIFactory2> dxgiFactory2 = nullptr;

        hr = _d3d11Device.As(&dxgiDevice);
        if(FAILED(hr)) {
            return false;
        }
        hr = dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf());
        if(FAILED(hr)) {
            return false;
        }
        hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(dxgiFactory1.GetAddressOf()));
        if(FAILED(hr)) {
            return false;
        }
        hr = dxgiFactory1.As(&dxgiFactory2);
        if(dxgiFactory2!=nullptr) {
            // 支持dx11.1
        } else {
            
        }


        return true;
    }

    void DX11Test::tick() {
    }
        
    void DX11Test::resize(uint32_t width, uint32_t height) {
        _width = width;
        _height = height;
    }

    void DX11Test::release() {
        delete this;
    }

    const char * DX11Test::title() {
        return "DX11Test";
    }
        
    uint32_t DX11Test::rendererType() {
        return 0;
    }


    void SwapchainDX11::onResize( uint32_t width, uint32_t height ) {
        _swapchainDesc.BufferDesc.Width = width;  
        _swapchainDesc.BufferDesc.Height = height;
        //

    }

}

ugi::DX11Test theapp;

UGIApplication* GetApplication() {
    return &theapp;
}