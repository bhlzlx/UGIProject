#include "DX11Test.h"
#include <cassert>
#include <cmath>
#include <LightWeightCommon/io/archive.h>

#if USE_IMGUI
#include <imgui/imgui.h>
#include <imgui/backends/imgui_impl_win32.h>
#include <imgui/backends/imgui_impl_dx11.h>
#endif

namespace ugi {

    bool DX11Test::initialize( void* _wnd, comm::IArchive* archive) {
        _device = new DeviceDX11();
        if(!_device->intialize()) {
            return false;
        }
    UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;
    #if defined( DEBUG ) || defined( _DEBUG )
        flags |= D3DCOMPILE_DEBUG; // add more debug output
    #endif
        ID3DBlob *vs_blob_ptr = NULL, *ps_blob_ptr = NULL, *error_blob = NULL;
        // COMPILE VERTEX SHADER
        HRESULT hr = D3DCompileFromFile(
        L"../dx11shader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "vs_main",
        "vs_5_0",
        flags,
        0,
        &vs_blob_ptr,
        &error_blob );
        if ( FAILED( hr ) ) {
            if ( error_blob ) {
                OutputDebugStringA( (char*)error_blob->GetBufferPointer() );
                error_blob->Release();
            }
            if ( vs_blob_ptr ) {
                vs_blob_ptr->Release();
            }
            assert( false );
        }

        // COMPILE PIXEL SHADER
        hr = D3DCompileFromFile(
        L"../dx11shader.hlsl",
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "ps_main",
        "ps_5_0",
        flags,
        0,
        &ps_blob_ptr,
        &error_blob );
        if (FAILED( hr )) {
            if ( error_blob ) {
                OutputDebugStringA( (char*)error_blob->GetBufferPointer() );
                error_blob->Release();
            }
            if ( ps_blob_ptr ) { ps_blob_ptr->Release(); }
            assert( false );
        }
        hr = _device->device()->CreateVertexShader(
            vs_blob_ptr->GetBufferPointer(),
            vs_blob_ptr->GetBufferSize(),
            NULL,
            &vs_
        );
        assert( SUCCEEDED( hr ) );
        hr = _device->device()->CreatePixelShader(
            ps_blob_ptr->GetBufferPointer(),
            ps_blob_ptr->GetBufferSize(),
            NULL,
            &ps_
        );
        assert( SUCCEEDED( hr ) );
        D3D11_INPUT_ELEMENT_DESC inputElementDesc[] = {
        { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        /*
        { "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEX", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        */
        };
        hr = _device->device()->CreateInputLayout(
            inputElementDesc,
            ARRAYSIZE( inputElementDesc ),
            vs_blob_ptr->GetBufferPointer(),
            vs_blob_ptr->GetBufferSize(),
            &inputLayout_
        );
        assert( SUCCEEDED( hr ) );
        float vertex_data_array[] = {
            0.0f,  0.5f,  0.0f, // point at top
            0.5f, -0.5f,  0.0f, // point at bottom-right
            -0.5f, -0.5f,  0.0f, // point at bottom-left
        };
        { /*** load mesh data into vertex buffer **/
            D3D11_BUFFER_DESC vertex_buff_descr     = {};
            vertex_buff_descr.ByteWidth             = sizeof( vertex_data_array );
            vertex_buff_descr.Usage                 = D3D11_USAGE_DEFAULT;
            vertex_buff_descr.BindFlags             = D3D11_BIND_VERTEX_BUFFER;
            D3D11_SUBRESOURCE_DATA sr_data          = { 0 };
            sr_data.pSysMem                         = vertex_data_array;
            HRESULT hr = _device->device()->CreateBuffer(
                &vertex_buff_descr,
                &sr_data,
                &vbo_
            );
            assert( SUCCEEDED( hr ) );
        }
#if USE_IMGUI
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;
        ImGui_ImplWin32_Init(HWND(_wnd));
        ImGui_ImplDX11_Init(_device->device(), _device->context());
#endif
        auto rasterizerDesc = CD3D11_RASTERIZER_DESC(
            D3D11_FILL_SOLID,
            D3D11_CULL_NONE,
            false,
            0, 0, 0, 0,
            false, false, false);
        _device->device()->CreateRasterizerState(&rasterizerDesc, &_rs);
        auto blendDesc = CD3D11_BLEND_DESC(CD3D11_DEFAULT());
        _device->device()->CreateBlendState(&blendDesc, &_bs);
        // Depth state
        auto depthDesc = CD3D11_DEPTH_STENCIL_DESC(
            FALSE, D3D11_DEPTH_WRITE_MASK_ZERO, D3D11_COMPARISON_LESS, 
            FALSE, D3D11_DEFAULT_STENCIL_READ_MASK, D3D11_DEFAULT_STENCIL_WRITE_MASK, 
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS, 
            D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_STENCIL_OP_KEEP, D3D11_COMPARISON_ALWAYS);
        _device->device()->CreateDepthStencilState(&depthDesc, &_ds);
        _swapchain = new SwapchainDX11(_device, (HWND)_wnd);
        _hwnd = _wnd;
        return true;
    }

    void DX11Test::tick() {
        if(!_swapchain->ready()) { // 交换链还没准备好
            return;
        }
        float channels[] = {
            0.8f, 0.8f, 0.2f, 1.0f
        };
        float depth = 1.0f;
        uint32_t stencil = 0xff;
        RECT winRect;
        GetClientRect((HWND)_hwnd, &winRect );
        D3D11_VIEWPORT viewport = {
            0.0f,
            0.0f,
            (FLOAT)(winRect.right - winRect.left),
            (FLOAT)(winRect.bottom - winRect.top),
            0.0f,
            1.0f
        };
        auto context = _device->context();
        context->RSSetViewports(1, &viewport);
        _swapchain->setRenderTarget(_device->context());
        _swapchain->clear(channels, depth, stencil);
        context->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
        context->IASetInputLayout(inputLayout_);
        uint32_t stride = sizeof(float) * 3;
        uint32_t offset = 0;
        context->IASetVertexBuffers(
            0,
            1,
            &vbo_,
            &stride,
            &offset
        );
        context->VSSetShader(vs_, NULL, 0 );
        context->PSSetShader(ps_, NULL, 0 );
        context->Draw(3, 0);
#if USE_IMGUI
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        bool show_demo_window = true;
        ImGui::ShowDemoWindow(&show_demo_window);
        ImGui::Begin("Another Window", &show_demo_window);   // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
        ImGui::Text("Hello from another window!");
        static int counter = 0;
        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::Text("counter = %d", counter);
        ImGui::End();
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
        _swapchain->present();
    }
        
    void DX11Test::resize(uint32_t width, uint32_t height) {
        _width = width;
        _height = height;
        _swapchain->onResize( width, height );
    }

    void DX11Test::release() {
    }

    const char * DX11Test::title() {
        return "DX11Test";
    }
        
    uint32_t DX11Test::rendererType() {
        return 0;
    }

    bool DeviceDX11::intialize() {
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

        ID3D11Device* device = nullptr;
        ID3D11DeviceContext* context = nullptr;
        for( uint32_t driverTypeIndex = 0; driverTypeIndex<numDriverTypes; ++driverTypeIndex ) {
            selectDriverType = driverType[driverTypeIndex];
            hr = D3D11CreateDevice(
                nullptr, selectDriverType, nullptr, createDeviceFlags, &featureLevels[0], numFeatureLevels, 
                D3D11_SDK_VERSION,
                &device, &selectFeatureLevel, &context
            );
            // 如果返回值是887a002d, 在控制面板里应用和功能里添加可用功能（图形工具）
            if( hr == E_INVALIDARG) { // 不支持11.1, 创建11.0
                hr = D3D11CreateDevice(
                    nullptr, selectDriverType, nullptr, createDeviceFlags, &featureLevels[1], 1,
                    D3D11_SDK_VERSION, 
                    &device, &selectFeatureLevel, &context
                );
                if(SUCCEEDED(hr)) {
                    _minorVersion = 0;
                    break;
                }
            } else {
                if(SUCCEEDED(hr)) {
                    _minorVersion = 1;
                    break;
                }
            }
        }
        if(FAILED(hr)) {
            MessageBox( NULL, L"Create D3D11 Device Failed!", 0, 0);
            return false;
        }
        // 确认支持的特性(实际根本没必要的，这代码是我抄来的，估计写这段代码的小哥没意识到)
        if ( selectFeatureLevel != D3D_FEATURE_LEVEL_11_0 && selectFeatureLevel != D3D_FEATURE_LEVEL_11_1) {
            MessageBox(0, L"Direct3D Feature Level 11 unsupported.", 0, 0);
            return false;
        }
        device->CheckMultisampleQualityLevels( DXGI_FORMAT_R8G8B8A8_UNORM, 4, &_msaaQuality );
        assert( _msaaQuality && "这个是一定会支持的！");
        //
        _device = device;
        _context = context;
    
        //
        return true;
    }

    ID3D11Device1* DeviceDX11::queryDevice1() {
        ID3D11Device1* device1 = nullptr;
        HRESULT hr = _context->QueryInterface( __uuidof(ID3D11Device1), (void**)&device1 );
        if(FAILED(hr)) {
            return nullptr;
        } else {
            return device1;
        }
    }

    ID3D11DeviceContext1* DeviceDX11::queryDeviceContext1() {
        ID3D11DeviceContext1* context1 = nullptr;
        HRESULT hr = _context->QueryInterface( __uuidof(ID3D11DeviceContext1), (void**)&context1);
        if(FAILED(hr)) {
            return nullptr;
        } else {
            return context1;
        }
    }

    IDXGISwapChain1* DeviceDX11::createSwapchain1(HWND hwnd, uint32_t width, uint32_t height, bool fullscreen) {
        IDXGIDevice* dxgiDevice = nullptr;
        IDXGIAdapter* dxgiAdapter = nullptr;
        IDXGIFactory1* dxgiFactory1 = nullptr;
        IDXGIFactory2* dxgiFactory2 = nullptr;
        HRESULT hr = _device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
        if(FAILED(hr)) {
            return nullptr;
        }
        hr = dxgiDevice->GetAdapter(&dxgiAdapter);
        if(FAILED(hr)) {
            return nullptr;
        }
        hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory1));
        if(FAILED(hr)) {
            return nullptr;
        }
        hr = dxgiFactory1->QueryInterface(__uuidof(IDXGIFactory2), (void**)&dxgiFactory2);
        if(FAILED(hr)) {
            return nullptr;
        }

        DXGI_SWAP_CHAIN_DESC1 sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.Width = width;
		sd.Height = height;
		sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		// 是否开启4倍多重采样？
        constexpr bool enalbe4xMsaa = true;
		if (enalbe4xMsaa) {
			sd.SampleDesc.Count = 4;
			sd.SampleDesc.Quality = _msaaQuality - 1;
		} else {
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
		}
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_SHADER_INPUT;
		sd.BufferCount = _bufferCount;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fd;
		fd.RefreshRate.Numerator = 60;
		fd.RefreshRate.Denominator = 1;
		fd.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		fd.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		fd.Windowed = fullscreen ? FALSE : TRUE;
		// 为当前窗口创建交换链
        IDXGISwapChain1* swapchain = nullptr;
        hr = dxgiFactory2->CreateSwapChainForHwnd( _device, hwnd, &sd, &fd, nullptr, &swapchain );
        if(FAILED(hr)) {
            return nullptr;
        } else {
            return swapchain;
        }
    }

    IDXGISwapChain* DeviceDX11::createSwapchain( HWND hwnd, uint32_t width, uint32_t height, bool fullscreen ) {
        IDXGIDevice* dxgiDevice = nullptr;
        IDXGIAdapter* dxgiAdapter = nullptr;
        IDXGIFactory1* dxgiFactory1 = nullptr;
        HRESULT hr = _device->QueryInterface(__uuidof(IDXGIDevice), (void**)&dxgiDevice);
        if(FAILED(hr)) {
            return nullptr;
        }
        hr = dxgiDevice->GetAdapter(&dxgiAdapter);
        if(FAILED(hr)) {
            return nullptr;
        }
        hr = dxgiAdapter->GetParent(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&dxgiFactory1));
        if(FAILED(hr)) {
            return nullptr;
        }
        DXGI_SWAP_CHAIN_DESC sd;
		ZeroMemory(&sd, sizeof(sd));
		sd.BufferDesc.Width = width;
		sd.BufferDesc.Height = height;
		sd.BufferDesc.RefreshRate.Numerator = 60;
		sd.BufferDesc.RefreshRate.Denominator = 1;
		sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		sd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		sd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		// 是否开启4倍多重采样？
        constexpr bool enalbe4xMsaa = true;
		if (enalbe4xMsaa) {
			sd.SampleDesc.Count = 4;
			sd.SampleDesc.Quality = _msaaQuality - 1;
		} else {
			sd.SampleDesc.Count = 1;
			sd.SampleDesc.Quality = 0;
		}
		sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		sd.BufferCount = _bufferCount;
		sd.OutputWindow = hwnd;
		sd.Windowed = fullscreen ? FALSE : TRUE;
		sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		sd.Flags = 0;
        IDXGISwapChain* swapchain = nullptr;
		hr = dxgiFactory1->CreateSwapChain( _device, &sd, &swapchain);
        if(FAILED(hr)) {
            return nullptr;
        } else {
            return swapchain;
        }
    }

    bool SwapchainDX11::onResize( uint32_t width, uint32_t height ) {
        if(_device->minorVersion()) {
            if(_swapchain1) {
                _swapchain1->Release();
                _swapchain1 = nullptr;
                if(!width || !height) {
                    return false;
                }
            }
            _swapchain1 = _device->createSwapchain1( _hwnd, width, height );
            for( uint32_t i = 0; i<BackbufferCount; ++i) {
                if(_renderTargetViews[i]) {
                    _renderTargetViews[i]->Release();
                    _renderBuffers[i]->Release();
                }
                auto hr = _swapchain1->GetBuffer(i, __uuidof(ID3D11Texture2D), (void**)&_renderBuffers[i]);
                if(FAILED(hr)) {
                    return false;
                }
                _renderTargetViews[i] = _device->createRenderTargetView(_renderBuffers[i]);
            }
        } else {
            if(_swapchain) {
                _swapchain->Release();
                _swapchain = nullptr;
                if(!width || !height) {
                    return false;
                }
            }
            _swapchain = _device->createSwapchain(_hwnd, width, height);
            for( uint32_t i = 0; i<BackbufferCount; ++i) {
                if(_renderTargetViews[i]) {
                    _renderTargetViews[i]->Release();
                    _renderBuffers[i]->Release();
                }
                _swapchain->GetBuffer(i, __uuidof(ID3D11Texture2D), (void**)&_renderBuffers[i]);
                _renderTargetViews[i] = _device->createRenderTargetView(_renderBuffers[i]);
            }
        }
        D3D11_TEXTURE2D_DESC colorDesc;
        _renderBuffers[0]->GetDesc(&colorDesc);
        if(_depthStencilBuffer && _depthStencilView) {
            _depthStencilView->Release();
            _depthStencilBuffer->Release();
        }
        // depth stencil
        D3D11_TEXTURE2D_DESC descDepth;
        ZeroMemory( &descDepth, sizeof(descDepth) );
        descDepth.Width = width;
        descDepth.Height = height;
        descDepth.MipLevels = 1;
        descDepth.ArraySize = 1;
        descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
        descDepth.SampleDesc.Count = colorDesc.SampleDesc.Count;
        descDepth.SampleDesc.Quality = colorDesc.SampleDesc.Quality;
        descDepth.Usage = D3D11_USAGE_DEFAULT;
        descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        descDepth.CPUAccessFlags = 0;
        descDepth.MiscFlags = 0;
        _depthStencilBuffer = _device->createTexture2D(descDepth);
        _depthStencilView = _device->createDepthStencilView(_depthStencilBuffer);
        return true;
    }

    void SwapchainDX11::clear(float(& channels)[4], float depth, uint32_t stencil ) {
        _device->context()->ClearRenderTargetView(_renderTargetViews[0], channels );
        _device->context()->ClearDepthStencilView(_depthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, depth, stencil);
    }

    void SwapchainDX11::present() {
        _swapchain->Present(0, 0);
    }

    void SwapchainDX11::setRenderTarget(ID3D11DeviceContext* context) {
        context->OMSetRenderTargets(1, &_renderTargetViews[0], _depthStencilView);
    }

    ID3D11Texture2D* DeviceDX11::createTexture2D( const D3D11_TEXTURE2D_DESC& desc ) const {
        ID3D11Texture2D* tex = nullptr;
        HRESULT hr;
        hr = _device->CreateTexture2D( &desc, nullptr, &tex );
        if(FAILED(hr)) {
            return nullptr;
        } else {
            return tex;
        }
    }
    
    ID3D11RenderTargetView* DeviceDX11::createRenderTargetView( ID3D11Texture2D* texture ) {
        HRESULT hr;
        ID3D11RenderTargetView* view = nullptr;
        hr = _device->CreateRenderTargetView(texture, nullptr, &view);
        if(FAILED(hr)) {
            return nullptr;
        } else {
            return view;
        }
    }

    ID3D11DepthStencilView* DeviceDX11::createDepthStencilView( ID3D11Texture2D* texture ) {
        HRESULT hr;
        ID3D11DepthStencilView* view = nullptr;
        hr = _device->CreateDepthStencilView(texture, nullptr, &view);
        if(FAILED(hr)) {
            return nullptr;
        } else {
            return view;
        }
    }

}

ugi::DX11Test theapp;

UGIApplication* GetApplication() {
    return &theapp;
}