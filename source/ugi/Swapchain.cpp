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

    VkSurfaceKHR Swapchain::createSurface( Device* device, void* window ) {
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
            (HWND)window
        };
        rst = vkCreateWin32SurfaceKHR( device->instance(), &surface_create_info, nullptr, &surface);
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
        vkGetPhysicalDeviceSurfaceSupportKHR( device->physicalDevice(), device->descriptor().queueFamilyIndices[0], surface, &supported );
        assert(supported);
        return surface;

    }

    bool Swapchain::initialize( Device* deviceVulkan, void* window, AttachmentLoadAction loadAction ) {
        _hwnd = window;
        _loadAction = loadAction;
        VkPhysicalDevice _physicalDevice = deviceVulkan->physicalDevice();
        VkDevice _device = deviceVulkan->device();
        // VkInstance _instance = _deviceVulkan->instance();
        //
        _surface = createSurface( deviceVulkan, window );
        if( !_surface ) {
            return false;
        }
        //
        vkDeviceWaitIdle( _device );
        //
        VkSwapchainCreateInfoKHR createInfo;
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        //
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &surfaceCapabilities) != VK_SUCCESS) {
            return false;
        }
        uint32_t formatCount;
        if (vkGetPhysicalDeviceSurfaceFormatsKHR( _physicalDevice, _surface, &formatCount, nullptr) != VK_SUCCESS) {
            return false;
        }
        if (formatCount == 0) {
            return false;
        }
        std::vector<VkSurfaceFormatKHR> surfaceFormats(formatCount);
        if (vkGetPhysicalDeviceSurfaceFormatsKHR( _physicalDevice, _surface, &formatCount, &surfaceFormats[0]) != VK_SUCCESS) {
            return false;
        }

        uint32_t nPresentMode;
        if ((vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &nPresentMode, nullptr) != VK_SUCCESS) ||
            (nPresentMode == 0)) {
            return false;
        }
        std::vector<VkPresentModeKHR> presentModes(nPresentMode);
        if (vkGetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, _surface, &nPresentMode, presentModes.data()) != VK_SUCCESS) {
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
        createInfo.surface = _surface;
        createInfo.flags = 0;
        createInfo.pNext = nullptr;
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
        createInfo.presentMode = presentMode;
        createInfo.preTransform = desiredTransform;

        VkSurfaceCapabilitiesKHR capabilities;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(_physicalDevice, _surface, &capabilities);
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        for (uint32_t flag = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; flag <= VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR; flag <<= 1) {   
            // VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR
            if (flag & capabilities.supportedCompositeAlpha) {
                createInfo.compositeAlpha = (VkCompositeAlphaFlagBitsKHR)flag;
                break;
            }
        }
        _createInfo = createInfo;
        //
        for( uint32_t i = 0; i<MaxFlightCount; ++i){
            _imageAvailSemaphores[i] = deviceVulkan->createSemaphore();
        }
        //
        return true;
    }

    bool Swapchain::resize( Device* device, uint32_t width, uint32_t height ) {
        _size = { width, height };
        return updateSwapchain(device);
    }


    bool Swapchain::updateSwapchain( Device* device ) {
        VkDevice dvcVk = device->device();
        const CommandQueue* queue = device->graphicsQueues()[0];
        queue->waitIdle();
        //
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR( device->physicalDevice(), _surface, &surfaceCapabilities) != VK_SUCCESS) {
            return false;
        }
        //
        _size.width = (_size.width + 1)&~(1);
        _size.height = (_size.height + 1)&~(1);
        //
        if (_size.width >= surfaceCapabilities.minImageExtent.width
            && _size.width <= surfaceCapabilities.maxImageExtent.width
            && _size.height >= surfaceCapabilities.minImageExtent.height
            && _size.height <= surfaceCapabilities.maxImageExtent.height
            ) {
            _createInfo.imageExtent = {
                (uint32_t)_size.width, (uint32_t)_size.height
            };
        }
        else {
            _createInfo.imageExtent = surfaceCapabilities.minImageExtent;
            _size.width = _createInfo.imageExtent.width;
            _size.height = _createInfo.imageExtent.height;
        }
        //
        _createInfo.surface = _surface;
        // 1. create swapchain object
        _createInfo.oldSwapchain = _swapchain;
        if ( _createInfo.imageExtent.width < 4 || _createInfo.imageExtent.height < 4 ) {
            return false;
        }
        VkResult rst = vkCreateSwapchainKHR( dvcVk, &_createInfo, nullptr, &_swapchain);
        if ( rst != VK_SUCCESS)
        {
            if( VK_ERROR_NATIVE_WINDOW_IN_USE_KHR == rst ){
                if (_createInfo.oldSwapchain != VK_NULL_HANDLE){
                    vkDestroySwapchainKHR(dvcVk, _createInfo.oldSwapchain, nullptr);
                    _createInfo.oldSwapchain = VK_NULL_HANDLE;
                }
                rst = vkCreateSwapchainKHR( dvcVk, &_createInfo, nullptr, &_swapchain);
            }else {
                return false;
            }
        }
        cleanup( device );
        //
        if (_createInfo.oldSwapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(dvcVk, _createInfo.oldSwapchain, nullptr);
        }
        // 2. retrieve the image attached on the swapchain
        _embedTextureCount = 0;
        rst = vkGetSwapchainImagesKHR(dvcVk, _swapchain, &_embedTextureCount, nullptr);
        if (!_embedTextureCount) {
            return false;
        }
        rst = vkGetSwapchainImagesKHR(dvcVk, _swapchain, &_embedTextureCount, _images);

        for( uint32_t embedImageIndex = 0; embedImageIndex < _embedTextureCount; ++ embedImageIndex ) {
            TextureDescription colorTexDesc;
            colorTexDesc.depth = 1;
            colorTexDesc.format = VkFormatToUGI(_createInfo.imageFormat);
            colorTexDesc.width = _createInfo.imageExtent.width;
            colorTexDesc.height = _createInfo.imageExtent.height;
            colorTexDesc.mipmapLevel = 1;
            colorTexDesc.arrayLayers = 1;
            colorTexDesc.type = TextureType::Texture2D;
            _embedTextures[embedImageIndex] = Texture::CreateTexture( device, _images[embedImageIndex], VK_NULL_HANDLE, colorTexDesc, ResourceAccessType::ColorAttachmentReadWrite );
            //
            ImageViewParameter vp;
            _embedColorView[embedImageIndex] = _embedTextures[embedImageIndex]->view( device, vp );
            if(!_dsv.texture) {
                TextureDescription depthStencilTexDesc = colorTexDesc;
                depthStencilTexDesc.format = UGIFormat::Depth32F;
                _dsvTex = Texture::CreateTexture( device, VK_NULL_HANDLE, VK_NULL_HANDLE, depthStencilTexDesc, ugi::ResourceAccessType::DepthStencilReadWrite );
                _dsv = _dsvTex->view( device, vp);
            }
            //
            // auto renderPassObjManager = _device->renderPassObjectManager();
            RenderPassDescription renderPassDesc;
            renderPassDesc.colorAttachmentCount = 1;
            renderPassDesc.colorAttachments[0].format = colorTexDesc.format;
            renderPassDesc.colorAttachments[0].loadAction = _loadAction;
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
            _renderPasses[embedImageIndex] = RenderPass::CreateRenderPass( device, renderPassDesc, &_embedColorView[embedImageIndex], _dsv );
        }
        return true;
    }

    void Swapchain::cleanup( Device* device ) {
        for( auto& rp : _renderPasses) {
            if( rp) {
                device->destroyRenderPass(rp);
                rp = nullptr;
            }
        }
        for( auto& colorTex : _embedTextures ) {
            if(colorTex) {
                device->destroyTexture(colorTex);
                colorTex = nullptr;
            }else {
                break;
            }
        }
        if( _dsvTex) {
            device->destroyTexture(_dsvTex);
            _dsvTex = nullptr;
            _dsv.texture = nullptr;
            _dsv.imageView = nullptr;
        }        
    }

    IRenderPass* Swapchain::renderPass( uint32_t _index ) {
        return _renderPasses[_index];
    }

    uint32_t Swapchain::acquireNextImage( Device* device, uint32_t flightIndex ) {
        if( !_swapchain ) {
            return UINT_MAX;
        }//
        Semaphore* imageAvailSemaphore = _imageAvailSemaphores[flightIndex];
        VkResult rst = vkAcquireNextImageKHR( device->device(), _swapchain, UINT64_MAX, VkSemaphore(*imageAvailSemaphore), VK_NULL_HANDLE, &_imageIndex);
        //
        switch (rst) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            // updateSwapchain();
            // 这一帧就舍弃了吧，实际上就算是重建交换链这帧也显示不出来
        default:
            _imageIndex = -1;
            _available = VK_FALSE;
            return false;
        }
        _available = VK_TRUE;
        //
        _flightIndex = flightIndex;
        //
        return _imageIndex;
    }

    bool Swapchain::present( Device* device, CommandQueue* graphicsQueue, Semaphore* semaphoreToWait ) {
        //
        VkSemaphore* psem = *semaphoreToWait;
        VkPresentInfoKHR presentInfo = {
            VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,            // VkStructureType              sType
            nullptr,                                    // const void                  *pNext
            1,                                            // uint32_t                     waitSemaphoreCount
            psem,                                        // const VkSemaphore           *pWaitSemaphores
            1,                                            // uint32_t                     swapchainCount
            &_swapchain,                                // const VkSwapchainKHR        *pSwapchains
            &_imageIndex,                                // const uint32_t              *pImageIndices
            nullptr                                        // VkResult                    *pResults
        };
        //
        VkResult result = vkQueuePresentKHR(*graphicsQueue, &presentInfo);

        switch (result) {
        case VK_SUCCESS:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_SUBOPTIMAL_KHR:
            return false;
        default:
            return false;
        }
        ++_flightIndex;
        _flightIndex = _flightIndex % MaxFlightCount;
        //
        return true;
    }

    Semaphore* Swapchain::imageAvailSemaphore() {
        return _imageAvailSemaphores[_flightIndex];
    }

}