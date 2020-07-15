#pragma once

#include "VulkanDeclare.h"
#include "UGITypes.h"
#include "UGIDeclare.h"
#include <Windows.h>

namespace ugi {

    class Swapchain {
    private:
        void*                               m_hwnd;
        VkSwapchainKHR                      m_swapchain;
        VkSwapchainCreateInfoKHR            m_createInfo;
        VkSurfaceKHR                        m_surface;
        // resource objects

        Semaphore*                          m_imageAvailSemaphores[MaxFlightCount];
        // Semaphore*                        m_renderCompleteSemaphores[MaxFlightCount];
        //
        //
        uint32_t                            m_embedTextureCount;
        VkImage                             m_images[4];
        Texture*                            m_embedTextures[4];
        Texture*                            m_depthStencilTexture;
        IRenderPass*                        m_renderPasses[4];
        // state description
        uint32_t                            m_imageIndex;
        uint32_t                            m_flightIndex;
        VkBool32                            m_available;
        Size<uint32_t>                      m_size;
        //
    private:
        VkSurfaceKHR createSurface( Device* _device, void* _window );
    public:
        Swapchain()
            : m_swapchain(VK_NULL_HANDLE)
            , m_createInfo {}
            , m_imageAvailSemaphores {}
            , m_embedTextureCount(4)
            , m_images { VK_NULL_HANDLE }
            , m_embedTextures{ nullptr }
            , m_depthStencilTexture(nullptr)
            , m_renderPasses {}
            , m_imageIndex(0)
            , m_flightIndex(0)
            , m_available(0)
            , m_size { 64, 64 }
        {
        }

        bool initialize( Device* _device, void* _window );
        bool resize( Device* _device, uint32_t _width, uint32_t _height );
        bool updateSwapchain( Device* _device );
        void cleanup( Device* _device );
        IRenderPass* renderPass( uint32_t _index );
        //
        uint32_t acquireNextImage( Device* _device, uint32_t _flightIndex );
        bool present( Device* _device, CommandQueue* _graphicsQueue, Semaphore* _semaphoreToWait );
        //
        Semaphore* imageAvailSemaphore();
    };

}