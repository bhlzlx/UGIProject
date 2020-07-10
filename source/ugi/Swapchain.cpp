#include "Swapchain.h"
#include "VulkanFunctionDeclare.h"
#include "Device.h"
#include "CommandQueue.h"
#include "Semaphore.h"
#include "Texture.h"
#include "UGITypeMapping.h"
#include "RenderPass.h"

#include <cassert>

#ifdef _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
extern PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#elif defined __ANDROID__
#include <vulkan/vulkan_android.h>
extern PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
#elif __linux__
#include <vulkan/vulkan_xlib.h>
extern PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
#endif

#include <vector>

namespace ugi {

    VkSurfaceKHR Swapchain::createSurface( Device* _device, void* _window ) {
        VkSurfaceKHR surface;
        VkResult rst = VK_ERROR_INVALID_EXTERNAL_HANDLE;
#ifdef _WIN32
        HINSTANCE hInst = ::GetModuleHandle(0);
        //
        VkWin32SurfaceCreateInfoKHR surface_create_info = {
            VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,  // VkStructureType                  sType
            nullptr,                                          // const void                      *pNext
            0,                                                // VkWin32SurfaceCreateFlagsKHR     flags
            hInst,
            (HWND)_window
        };
        rst = vkCreateWin32SurfaceKHR( _device->instance(), &surface_create_info, nullptr, &surface);
#elif defined __ANDROID__
        VkAndroidSurfaceCreateInfoKHR surface_create_info = {
            VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,  // sType
            nullptr,// pNext
            0, // flag
            (ANativeWindow*)_hwnd
        };
        rst = vkCreateAndroidSurfaceKHR(m_instance, &surface_create_info, nullptr, &surface);
#elif defined __linux__
        VkXlibSurfaceCreateInfoKHR surface_create_info = {
            VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,  // sType
            nullptr,// pNext
            0, // flag
            (Display*)_hwnd
        };
        rst = vkCreateXlibSurfaceKHR(m_instance, &surface_create_info, nullptr, &surface);
#endif
        if (rst != VK_SUCCESS){
            return VK_NULL_HANDLE;
        }
        VkBool32 supported = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR( _device->physicalDevice(), _device->descriptor().queueFamilyIndices[0], surface, &supported );
        assert(supported);
        return surface;

    }

    bool Swapchain::initialize( Device* _deviceVulkan, void* _window ) {
        m_hwnd = _window;
        VkPhysicalDevice _physicalDevice = _deviceVulkan->physicalDevice();
        VkDevice _device = _deviceVulkan->device();
        // VkInstance _instance = _deviceVulkan->instance();
        //
        m_surface = createSurface( _deviceVulkan, _window );
        if( !m_surface ) {
            return false;
        }
        //
        vkDeviceWaitIdle( _device );
        //
        VkSwapchainCreateInfoKHR createInfo;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        //
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, m_surface, &surfaceCapabilities) != VK_SUCCESS) {
            return false;
        }
        uint32_t formatCount;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR( _physicalDevice, m_surface, &formatCount, nullptr) != VK_SUCCESS) {
            return false;
        }
        if (formatCount == 0) {
            return false;
        }
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR( _physicalDevice, m_surface, &formatCount, &surfaceFormats[0]) != VK_SUCCESS) {
            return false;
        }

        uint32_t nPresentMode;
        if ((vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, m_surface, &nPresentMode, nullptr) != VK_SUCCESS) ||
            (nPresentMode == 0)) {
            return false;
        }
        std::vector<VkPresentModeKHR> presentModes(nPresentMode);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, m_surface, &nPresentMode, presentModes.data()) != VK_SUCCESS) {
            return false;
        }
        //
        if (surfaceCapabilities.maxImageCount == 0) {
            createInfo.minImageCount = surfaceCapabilities.minImageCount < MaxFlightCount ? MaxFlightCount : surfaceCapabilities.minImageCount;
        } else {
            createInfo.minImageCount = surfaceCapabilities.maxImageCount < MaxFlightCount ? surfaceCapabilities.maxImageCount : MaxFlightCount;
        }
        // 2.
        VkSurfaceFormatKHR desiredFormat = surfaceFormats[0];
        if ((surfaceFormats.size() == 1) && (surfaceFormats[0].format == VK_FORMAT_UNDEFINED)) {
            desiredFormat = { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR };
        }
//        else
//        {
//            for (VkSurfaceFormatKHR &surfaceFormat : surfaceFormats)
//            {
//                if (surfaceFormat.format == VK_FORMAT_R8G8B8A8_UNORM)
//                {
//                    desiredFormat = surfaceFormat;
//                    break;
//                }
//            }
//        }

        VkSurfaceTransformFlagBitsKHR desiredTransform;
        VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;

        if (surfaceCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR) {
            desiredTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
        } else {
            desiredTransform = surfaceCapabilities.currentTransform;
        }            
        // 6.
        // FIFO present mode is always available
        // MAILBOX is the lowest latency V-Sync enabled mode (something like triple-buffering) so use it if available
//         for (VkPresentModeKHR &pmode : presentModes)
//         {
//             if (pmode == VK_PRESENT_MODE_MAILBOX_KHR)
//             {
//                 presentMode = pmode; break;
//             }
//         }
// 
//         if (presentMode != VK_PRESENT_MODE_MAILBOX_KHR)
//         {
//             for (VkPresentModeKHR &pmode : presentModes)
//             {
//                 if (pmode == VK_PRESENT_MODE_FIFO_KHR)
//                 {
//                     presentMode = pmode; break;
//                 }
//             }
//         }
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        //_swapchainCI.minImageCount = _swapchainCI.minImageCount;
        createInfo.imageFormat = desiredFormat.format;
        createInfo.imageColorSpace = desiredFormat.colorSpace;
        createInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        createInfo.imageArrayLayers = 1;
        createInfo.clipped = VK_TRUE;
        createInfo.surface = m_surface;
        createInfo.flags = 0;
        createInfo.pNext = nullptr;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.presentMode = presentMode;
        createInfo.preTransform = desiredTransform;

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, m_surface, &capabilities);
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        for (uint32_t flag = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; flag <= VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR; flag <<= 1) {   
            // VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
            if (flag & capabilities.supportedCompositeAlpha) {
                createInfo.compositeAlpha = (VkCompositeAlphaFlagBitsKHR)flag;
                break;
            }
        }
        m_createInfo = createInfo;
        //
        for( uint32_t i = 0; i<MaxFlightCount; ++i){
            m_imageAvailSemaphores[i] = _deviceVulkan->createSemaphore();
            // m_renderCompleteSemaphores[i] = _deviceVulkan->createSemaphore();
        }
        //
        return true;
    }

    bool Swapchain::resize( Device* _device, uint32_t _width, uint32_t _height ) {
        m_size = { _width, _height };
        return updateSwapchain(_device);
    }


    bool Swapchain::updateSwapchain( Device* _device ) {
        VkDevice device = _device->device();
        const CommandQueue* queue = _device->graphicsQueues()[0];
        queue->waitIdle();
        //
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR( _device->physicalDevice(), m_surface, &surfaceCapabilities) != VK_SUCCESS) {
            return false;
        }
        //
        m_size.width = (m_size.width + 1)&~(1);
        m_size.height = (m_size.height + 1)&~(1);
        //
        if (m_size.width >= surfaceCapabilities.minImageExtent.width
            && m_size.width <= surfaceCapabilities.maxImageExtent.width
            && m_size.height >= surfaceCapabilities.minImageExtent.height
            && m_size.height <= surfaceCapabilities.maxImageExtent.height
            ) {
            m_createInfo.imageExtent = {
                (uint32_t)m_size.width, (uint32_t)m_size.height
            };
        }
        else {
            m_createInfo.imageExtent = surfaceCapabilities.minImageExtent;
            m_size.width = m_createInfo.imageExtent.width;
            m_size.height = m_createInfo.imageExtent.height;
        }
        //
        m_createInfo.surface = m_surface;
        // 1. create swapchain object
        m_createInfo.oldSwapchain = m_swapchain;
        if ( m_createInfo.imageExtent.width < 4 || m_createInfo.imageExtent.height < 4 ) {
            return false;
        }
        VkResult rst = vkCreateSwapchainKHR( device, &m_createInfo, nullptr, &m_swapchain);
        if ( rst != VK_SUCCESS)
        {
            if( VK_ERROR_NATIVE_WINDOW_IN_USE_KHR == rst ){
                if (m_createInfo.oldSwapchain != VK_NULL_HANDLE){
                    vkDestroySwapchainKHR(device, m_createInfo.oldSwapchain, nullptr);
                    m_createInfo.oldSwapchain = VK_NULL_HANDLE;
                }
                rst = vkCreateSwapchainKHR( device, &m_createInfo, nullptr, &m_swapchain);
            }else {
                return false;
            }
        }
        cleanup( _device );
        //
        if (m_createInfo.oldSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(device, m_createInfo.oldSwapchain, nullptr);
        }
        // 2. retrieve the image attached on the swapchain
        m_embedTextureCount = 0;
        rst = vkGetSwapchainImagesKHR(device, m_swapchain, &m_embedTextureCount, nullptr);
        if (!m_embedTextureCount) {
            return false;
        }
        rst = vkGetSwapchainImagesKHR(device, m_swapchain, &m_embedTextureCount, m_images);

        for( uint32_t embedImageIndex = 0; embedImageIndex < m_embedTextureCount; ++ embedImageIndex ) {
            TextureDescription colorTexDesc;
            colorTexDesc.depth = 1;
            colorTexDesc.format = VkFormatToUGI(m_createInfo.imageFormat);
            colorTexDesc.width = m_createInfo.imageExtent.width;
            colorTexDesc.height = m_createInfo.imageExtent.height;
            colorTexDesc.mipmapLevel = 1;
            colorTexDesc.arrayLayers = 1;
            colorTexDesc.type = TextureType::Texture2D;
            m_embedTextures[embedImageIndex] = Texture::CreateTexture( _device, m_images[embedImageIndex], VK_NULL_HANDLE, colorTexDesc, ResourceAccessType::ColorAttachmentReadWrite );
            TextureDescription depthStencilTexDesc = colorTexDesc;
            depthStencilTexDesc.format = UGIFormat::Depth32F;
            m_depthStencilTextures[embedImageIndex] = Texture::CreateTexture( _device, VK_NULL_HANDLE, VK_NULL_HANDLE, depthStencilTexDesc, ugi::ResourceAccessType::DepthStencilReadWrite );
            //
            // auto renderPassObjManager = _device->renderPassObjectManager();
            RenderPassDescription renderPassDesc;
            renderPassDesc.colorAttachmentCount = 1;
            renderPassDesc.colorAttachments[0].format = colorTexDesc.format;
            renderPassDesc.colorAttachments[0].loadAction = AttachmentLoadAction::Clear;
            renderPassDesc.colorAttachments[0].multisample = MultiSampleType::MsaaNone;
            renderPassDesc.colorAttachments[0].initialAccessType = ResourceAccessType::Present; ///> 这里就是说这个swapchain只有一个作用就是渲染完就显示，永远不用作其它类型
            renderPassDesc.colorAttachments[0].finalAccessType = ResourceAccessType::Present;
            renderPassDesc.depthStencil.format = UGIFormat::Depth32F;
            renderPassDesc.depthStencil.loadAction = AttachmentLoadAction::Clear;
            renderPassDesc.depthStencil.multisample = MultiSampleType::MsaaNone;
            renderPassDesc.depthStencil.initialAccessType = ResourceAccessType::DepthStencilReadWrite; ///>
            renderPassDesc.depthStencil.finalAccessType = ResourceAccessType::DepthStencilReadWrite;
            renderPassDesc.inputAttachmentCount = 0;
            //
            m_renderPasses[embedImageIndex] = RenderPass::CreateRenderPass( _device, renderPassDesc, &m_embedTextures[embedImageIndex], m_depthStencilTextures[embedImageIndex] );
        }
        return true;
    }

    void Swapchain::cleanup( Device* _device ) {
        // vkDestroyRenderPass( )
    }

    IRenderPass* Swapchain::renderPass( uint32_t _index ) {
        return m_renderPasses[_index];
    }

    uint32_t Swapchain::acquireNextImage( Device* _device, uint32_t _flightIndex ) {
        if( !m_swapchain ) {
            return UINT_MAX;
        }//
        VkResult rst = vkAcquireNextImageKHR( _device->device(), m_swapchain, UINT64_MAX, *m_imageAvailSemaphores[_flightIndex], VK_NULL_HANDLE, &m_imageIndex);
        //
        switch (rst) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            // updateSwapchain();
            // 这一帧就舍弃了吧，实际上就算是重建交换链这帧也显示不出来
        default:
            m_imageIndex = -1;
            m_available = VK_FALSE;
            return false;
        }
        m_available = VK_TRUE;
        //
        m_flightIndex = _flightIndex;
        //
        return m_imageIndex;
    }

    bool Swapchain::present( Device* _device, CommandQueue* _graphicsQueue, Semaphore* _semaphoreToWait ) {
        //
        VkSemaphore* psem = *_semaphoreToWait;
        VkPresentInfoKHR presentInfo = {
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,            // VkStructureType              sType
            nullptr,                                    // const void                  *pNext
            1,                                            // uint32_t                     waitSemaphoreCount
            psem,                                        // const VkSemaphore           *pWaitSemaphores
            1,                                            // uint32_t                     swapchainCount
            &m_swapchain,                                // const VkSwapchainKHR        *pSwapchains
            &m_imageIndex,                                // const uint32_t              *pImageIndices
            nullptr                                        // VkResult                    *pResults
        };
        //
        VkResult result = vkQueuePresentKHR(*_graphicsQueue, &presentInfo);

        switch (result) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
            return false;
        default:
            return false;
        }
        ++m_flightIndex;
        m_flightIndex = m_flightIndex % MaxFlightCount;
        //
        return true;
    }

    Semaphore* Swapchain::imageAvailSemaphore() {
        return m_imageAvailSemaphores[m_flightIndex];
    }

}