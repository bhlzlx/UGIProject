#pragma once

#include "vulkan_declare.h"
#include "ugi_vulkan_private.h"
#include "ugi_declare.h"
#include <Windows.h>

namespace ugi {

    class Swapchain {
    private:
        void*                               _hwnd;
        VkSwapchainKHR                      _swapchain;
        VkSwapchainCreateInfoKHR            _createInfo;
        VkSurfaceKHR                        _surface;
        AttachmentLoadAction                _loadAction;
        // resource objects

        Semaphore*                          _imageAvailSemaphores[MaxFlightCount];
        // Semaphore*                        _renderCompleteSemaphores[MaxFlightCount];
        //
        //
        uint32_t                            _embedTextureCount;
        VkImage                             _images[4];
        Texture*                            _embedColors[4];
        image_view_t                        _embedColorViews[4];
        Texture*                            _dsTexture;
        image_view_t                        _dsv;
        IRenderPass*                        _renderPasses[4];
        // state description
        uint32_t                            _imageIndex;
        uint32_t                            _flightIndex;
        VkBool32                            _available;
        extent_2d_t<uint32_t>                      _size;
        //
    private:
        VkSurfaceKHR createSurface( Device* device, void* window );
    public:
        Swapchain()
            : _swapchain(VK_NULL_HANDLE)
            , _createInfo{}
            , _imageAvailSemaphores{}
            , _embedTextureCount(4)
            , _images{ VK_NULL_HANDLE }
            , _embedColors{}
            , _embedColorViews{}
            , _dsTexture(nullptr)
            , _dsv{}
            , _renderPasses{}
            , _imageIndex(0)
            , _flightIndex(0)
            , _available(0)
            , _size{ 64, 64 }
        {
        }

        bool initialize( Device* _device, void* _window, AttachmentLoadAction loadAction );
        bool resize( Device* _device, uint32_t _width, uint32_t _height );
        bool updateSwapchain( Device* _device );
        void cleanup( Device* _device );
        IRenderPass* renderPass( uint32_t _index );
        //
        uint32_t acquireNextImage( Device* device, uint32_t flightIndex );
        bool present( Device* _device, CommandQueue* graphicsQueue, Semaphore* semaphoreToWait );
        //
        Semaphore* imageAvailSemaphore();
    };

}