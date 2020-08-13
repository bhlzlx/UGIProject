#pragma once

#include "VulkanDeclare.h"
#include "UGITypes.h"
#include "UGIDeclare.h"
#include <Windows.h>

namespace ugi {

    class Swapchain {
    private:
        void*                               _hwnd;
        VkSwapchainKHR                      _swapchain;
        VkSwapchainCreateInfoKHR            _createInfo;
        VkSurfaceKHR                        _surface;
        // resource objects

        Semaphore*                          _imageAvailSemaphores[MaxFlightCount];
        // Semaphore*                        _renderCompleteSemaphores[MaxFlightCount];
        //
        //
        uint32_t                            _embedTextureCount;
        VkImage                             _images[4];
        Texture*                            _embedTextures[4];
        Texture*                            _depthStencilTexture;
        IRenderPass*                        _renderPasses[4];
        // state description
        uint32_t                            _imageIndex;
        uint32_t                            _flightIndex;
        VkBool32                            _available;
        Size<uint32_t>                      _size;
        //
    private:
        VkSurfaceKHR createSurface( Device* device, void* window );
    public:
        Swapchain()
            : _swapchain(VK_NULL_HANDLE)
            , _createInfo {}
            , _imageAvailSemaphores {}
            , _embedTextureCount(4)
            , _images { VK_NULL_HANDLE }
            , _embedTextures{ nullptr }
            , _depthStencilTexture(nullptr)
            , _renderPasses {}
            , _imageIndex(0)
            , _flightIndex(0)
            , _available(0)
            , _size { 64, 64 }
        {
        }

        bool initialize( Device* _device, void* _window );
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