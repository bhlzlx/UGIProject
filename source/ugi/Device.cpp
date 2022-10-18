#include "Device.h"

#include "VulkanFunctionDeclare.h"
#include "VulkanDebugConfigurator.h"
//
#include <vector>
#include <set>
#include <map>
#include <cstdint>
#include <cassert>
#include <utility>
#include <algorithm>

#include <vk_mem_alloc.h>

#include "Buffer.h"
#include "Texture.h"
#include "Swapchain.h"
#include "Semaphore.h"
#include "RenderPass.h"
#include "CommandQueue.h"
#include "UGITypeMapping.h"
#include "Argument.h"
#include "Pipeline.h"
#include "Descriptor.h"
#include "Drawable.h"
#include "UniformBuffer.h"

#ifdef _WIN32
#include <Windows.h>
#endif
#ifdef __ANDROID__
#include <dlfcn.h>
#elif defined __linux__
#include <dlfcn.h>
#include <X11/Xlib.h>
#endif

#ifdef _WIN32
#define VULKAN_LIBRARY_NAME "vulkan-1.dll"
#define SHADER_COMPILER_LIBRARY_NAME "VkShaderCompiler.dll"
#else
#define VULKAN_LIBRARY_NAME "libvulkan.so"
#define SHADER_COMPILER_LIBRARY_NAME "libVkShaderCompiler.so"
#endif

#undef REGIST_VULKAN_FUNCTION
#ifdef _WIN32
#define REGIST_VULKAN_FUNCTION( FUNCTION ) FUNCTION = reinterpret_cast<PFN_##FUNCTION>(GetProcAddress( (HMODULE)library, #FUNCTION ));
#elif defined __ANDROID__
#define REGIST_VULKAN_FUNCTION( FUNCTION ) FUNCTION = reinterpret_cast<PFN_##FUNCTION>(dlsym( library, #FUNCTION ));
#elif __linux__ 
#define REGIST_VULKAN_FUNCTION( FUNCTION ) FUNCTION = reinterpret_cast<PFN_##FUNCTION>(dlsym( library, #FUNCTION ));
#endif

#ifdef _WIN32
#include <Windows.h>
#include <vulkan/vulkan_win32.h>
PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#elif defined __ANDROID__
#include <vulkan/vulkan_android.h>
PFN_vkCreateAndroidSurfaceKHR vkCreateAndroidSurfaceKHR;
#elif __linux__
#include <vulkan/vulkan_xlib.h>
PFN_vkCreateXlibSurfaceKHR vkCreateXlibSurfaceKHR;
#endif

#ifdef _WIN32
#include <Windows.h>
#define OpenLibrary( name ) (void*)::LoadLibraryA(name)
#define CloseLibrary( library ) ::FreeLibrary((HMODULE)library)
#define GetExportAddress( libray, function ) ::GetProcAddress( (HMODULE)libray, function )
#else
#include <dlfcn.h>
#define OpenLibrary( name ) dlopen(name , RTLD_NOW | RTLD_LOCAL)
#define CloseLibrary( library ) dlclose((void*)library)
#define GetExportAddress( libray, function ) dlsym( (void*)libray, function )
#endif

namespace ugi {

    VkPhysicalDeviceType MappingDeviceType( ugi::GRAPHICS_DEVICE_TYPE _type ) {
        switch( _type ) {
            case GRAPHICS_DEVICE_TYPE::INTEGATED: {
                return VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
            }
            case GRAPHICS_DEVICE_TYPE::DISCRETE: {
                return VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
            }
            case GRAPHICS_DEVICE_TYPE::SOFTWARE: {
                return VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_CPU;
            }
            case GRAPHICS_DEVICE_TYPE::OTHER: {
                return VkPhysicalDeviceType::VK_PHYSICAL_DEVICE_TYPE_OTHER;
            }
        }
        return VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    }

    DebugReporterVk debugReporter;

    Device*
    RenderSystem::createDevice( const DeviceDescriptor& _descriptor, common::IArchive* archive) {
        m_deviceDescriptorVk = _descriptor;
        auto library = OpenLibrary(VULKAN_LIBRARY_NAME);
        if (library == NULL) {
            return nullptr;
        }
#include "VulkanFunctionRegistList.h"
#ifdef _WIN32
        REGIST_VULKAN_FUNCTION(vkCreateWin32SurfaceKHR)
#elif defined __ANDROID__
        REGIST_VULKAN_FUNCTION(vkCreateAndroidSurfaceKHR)
#elif defined __linux__
        REGIST_VULKAN_FUNCTION(vkCreateXlibSurfaceKHR)
#endif        
        if (!vkGetInstanceProcAddr || !vkCreateInstance || !vkAcquireNextImageKHR) {
            return nullptr;
        }
// #ifndef NDEBUG
//         struct FUNCTION_ITEM {
//             const char* name;
//             const void* const address;
//         };
//         
// #undef REGIST_VULKAN_FUNCTION
//  #define REGIST_VULKAN_FUNCTION(FUNCTION) { #FUNCTION, (const void* const)FUNCTION },
//          FUNCTION_ITEM apiList[] = {
//              #include "VulkanFunctionRegistList.h"
//          };
// #endif
        createVulkanInstance( _descriptor.debugLayer );
        m_deviceDescriptorVk.archive = archive;
        // setup debug
        if (_descriptor.debugLayer) {
            debugReporter.setupDebugReport( m_deviceDescriptorVk.instance );
        }
        // 
        selectVulkanPhysicalDevice();
        createVulkanSurface();
        //
        return createVulkanDevice();
    }

    void RenderSystem::createVulkanInstance(bool validation) {
        const char APPLICATION_NAME[] = "UGI - VULKAN";
        const char ENGINE_NAME[] = "UGI - VULKAN ENGINE";
        VkApplicationInfo applicationInfo; {
            applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            applicationInfo.pNext = nullptr;
            applicationInfo.pApplicationName = APPLICATION_NAME;
            applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            applicationInfo.apiVersion = VK_MAKE_VERSION(1, 0, 0);
            applicationInfo.pEngineName = ENGINE_NAME;
            applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        }
        // ============= Get Layer List =============
        static const auto LayerList = {
            // "VK_LAYER_LUNARG_parameter_validation",
            // "VK_LAYER_LUNARG_object_tracker",
            // "VK_LAYER_LUNARG_core_validation",
            // "VK_LAYER_LUNARG_device_limits",
            // "VK_LAYER_LUNARG_image",
            // "VK_LAYER_LUNARG_swapchain",
            // "VK_LAYER_GOOGLE_threading",
            // "VK_LAYER_GOOGLE_unique_objects",
            "VK_LAYER_LUNARG_standard_validation",
            "VK_LAYER_KHRONOS_validation"
        };
        std::vector< const char* > vecLayers;
        // not all pre defined layers are supported by your graphics card
        std::vector< VkLayerProperties  > vecLayerProps;
        uint32_t count = 0;
        vkEnumerateInstanceLayerProperties(&count, nullptr);
        if (count) {
            vecLayerProps.resize(count);
            vkEnumerateInstanceLayerProperties(&count, &vecLayerProps[0]);
        }

        if (validation) {
            for (auto& layer : vecLayerProps)
            {
                for (auto& enableLayer : LayerList)
                {
                    if (strcmp(layer.layerName, enableLayer) == 0)
                    {
                        vecLayers.push_back(enableLayer);
                    }
                }
            }
        }        
        // ============= Get Extention List =============
        count = 0;
        std::vector< VkExtensionProperties > vecExtProps;
        std::vector<const char*> exts;
        vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
        if (count) {
            vecExtProps.resize(count);
            vkEnumerateInstanceExtensionProperties(nullptr, &count, &vecExtProps[0]);
        }
        for (auto& ext : vecExtProps) {
            exts.push_back(ext.extensionName);
        }
        // ============= Create Instance Object =============
        VkInstanceCreateInfo info; {
            info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            info.flags = 0;
            info.pNext = nullptr;
            info.pApplicationInfo = &applicationInfo;
            info.enabledLayerCount = static_cast<uint32_t>(vecLayers.size());
            info.ppEnabledLayerNames = vecLayers.data();
            info.enabledExtensionCount = static_cast<uint32_t>(exts.size());
            info.ppEnabledExtensionNames = exts.data();
        }
        vkCreateInstance(&info, nullptr, &m_deviceDescriptorVk.instance);
    }


    void RenderSystem::selectVulkanPhysicalDevice() {
        uint32_t physicalDeviceCount = 0;
        std::vector<VkPhysicalDevice>  physicalDevices;
        auto rst = vkEnumeratePhysicalDevices( m_deviceDescriptorVk.instance, &physicalDeviceCount, nullptr);
        if (rst != VK_SUCCESS || physicalDeviceCount == 0) {
            throw DeviceCreationException( DeviceCreationException::EXCEPTION_NO_PHYSICAL_DEVICE_AVAILABLE );
            return;
        }
        physicalDevices.resize(physicalDeviceCount);
        rst = vkEnumeratePhysicalDevices( m_deviceDescriptorVk.instance, &physicalDeviceCount, &physicalDevices[0]);
        // select a device that match the device type
        std::vector< VkPhysicalDeviceProperties > vecProps;
        for (auto& phyDevice : physicalDevices)
        {
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(phyDevice, &props);
            uint32_t major_version = VK_VERSION_MAJOR(props.apiVersion);
            // uint32_t minor_version = VK_VERSION_MINOR(props.apiVersion);
            // uint32_t patch_version = VK_VERSION_PATCH(props.apiVersion);
            if (major_version >= 1 && props.deviceType == MappingDeviceType( m_deviceDescriptorVk.deviceType) ) {
                m_deviceDescriptorVk.properties = props;
                m_deviceDescriptorVk.physicalDevice = phyDevice;
                return;
            }
            vecProps.push_back(props);
        }
        // cannot find a device match the device type
        m_deviceDescriptorVk.physicalDevice = physicalDevices[0];
        vkGetPhysicalDeviceProperties(m_deviceDescriptorVk.physicalDevice, &m_deviceDescriptorVk.properties );
        return;
    }

    Device* RenderSystem::createVulkanDevice() {

        auto physicalDevice = m_deviceDescriptorVk.physicalDevice;

        assert( m_deviceDescriptorVk.surface );
        //
        struct QueueItem {
            union {
                uint64_t id;
                struct {
                    uint32_t queueFamily;
                    uint32_t queueIndex;
                };
            };
        };

        std::vector<uint64_t> requestedGraphicsQueueItems;
        std::vector<uint64_t> requestedTransferQueueItems;
        std::vector<uint64_t> requestedAllQueueItems;

        std::vector<VkDeviceQueueCreateInfo> infos;
        //
        uint32_t queueFamilyPropertyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyPropertyCount, nullptr );
        if( !queueFamilyPropertyCount ) {
            throw DeviceCreationException( DeviceCreationException::EXCEPTION_NO_QUEUE_FAMILY_AVAILABLE );
            return nullptr;
        }
        std::vector<VkQueueFamilyProperties> familyProperties( queueFamilyPropertyCount );
        vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyPropertyCount, familyProperties.data() );
        // request graphics queues
        for (uint32_t familyIndex = 0; familyIndex < familyProperties.size() && requestedGraphicsQueueItems.size() < m_deviceDescriptorVk.graphicsQueueCount; ++familyIndex) {
            const VkQueueFamilyProperties& property = familyProperties[familyIndex];
            if (property.queueCount && property.queueFlags & VK_QUEUE_GRAPHICS_BIT && property.queueFlags & VK_QUEUE_COMPUTE_BIT) {
                VkBool32 supportPresent = VK_FALSE;
                vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, familyIndex, m_deviceDescriptorVk.surface, &supportPresent);
                if (supportPresent == VK_TRUE) {
                    QueueItem item;
                    item.queueFamily = familyIndex;
                    for (uint16_t index = 0; index < property.queueCount && requestedGraphicsQueueItems.size() < m_deviceDescriptorVk.graphicsQueueCount; ++index) {
                        item.queueIndex = index;
                        if( std::find( requestedAllQueueItems.begin(), requestedAllQueueItems.end(), item.id ) == requestedAllQueueItems.end() ) {
                            requestedAllQueueItems.push_back(item.id);
                            requestedGraphicsQueueItems.push_back(item.id);
                        }
                    }
                }
            }
        }
        // request transfer queues
        for (uint32_t familyIndex = 0; familyIndex < familyProperties.size()&& requestedTransferQueueItems.size() < m_deviceDescriptorVk.graphicsQueueCount; ++familyIndex) {
            const VkQueueFamilyProperties& property = familyProperties[familyIndex];
            if (property.queueCount && (property.queueFlags & VK_QUEUE_TRANSFER_BIT || property.queueFlags & VK_QUEUE_GRAPHICS_BIT)) {
                QueueItem item;
                item.queueFamily = familyIndex;
                for (uint16_t index = 0; index < property.queueCount && requestedTransferQueueItems.size() < m_deviceDescriptorVk.graphicsQueueCount; ++index) {
                    item.queueIndex = index;
                    if( std::find( requestedAllQueueItems.begin(), requestedAllQueueItems.end(), item.id ) == requestedAllQueueItems.end() ) {
                        requestedAllQueueItems.push_back(item.id);
                        requestedTransferQueueItems.push_back(item.id);
                    }
                }
            }
        }
        // generate the request information
        std::map< uint32_t, uint32_t > records;
        for (auto& req : requestedAllQueueItems) {
            QueueItem item = { req };
            uint32_t family = item.queueFamily;
            if (records.find(family) == records.end()) {
                records[family] = 1;
            } else {
                ++records[family];
            }
        }
        std::vector<VkDeviceQueueCreateInfo> deviceQueueCreateInfos;
        const static float Priorities[] = {
            1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
        };
        for (auto& record : records) {
            VkDeviceQueueCreateInfo info;
            info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            info.pNext = nullptr;
            info.flags = 0;
            info.pQueuePriorities = &Priorities[0];
            info.queueCount = record.second;
            info.queueFamilyIndex = record.first;
            deviceQueueCreateInfos.push_back(info);
        }
        //
        std::vector< const char* > layers;
        std::vector<VkLayerProperties> vecLayerProps;
        std::vector< const char* > exts;
        std::vector<VkExtensionProperties> vecExtsProps;
        uint32_t count;
        vkEnumerateDeviceLayerProperties( physicalDevice, &count, nullptr);
        if (count) {
            vecLayerProps.resize(count);
            vkEnumerateDeviceLayerProperties(physicalDevice, &count, &vecLayerProps[0]);
        }
        for (auto& layer : vecLayerProps){
            layers.push_back(layer.layerName);
        }

        count = 0;
        vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &count, nullptr);
        vecExtsProps.resize(count);
        vkEnumerateDeviceExtensionProperties( physicalDevice, nullptr, &count, &vecExtsProps[0]);
        for (auto& ext : vecExtsProps)
        {
            exts.push_back(ext.extensionName);
        }
        VkPhysicalDeviceFeatures features;
        vkGetPhysicalDeviceFeatures( physicalDevice, &features);

        const char * deviceExts[] = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME
        };
        //
        VkDeviceCreateInfo deviceCreateInfo = {
            VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO, // VkStructureType sType
            nullptr, // const void *pNext
            0, // VkDeviceCreateFlags flags
            static_cast<uint32_t>( deviceQueueCreateInfos.size()), // uint32_t queueCreateInfoCount
            deviceQueueCreateInfos.data(), // const VkDeviceQueueCreateInfo     *pQueueCreateInfos
            0,//1,// deviceLayers.size(),//0,
            nullptr,//&mgdLayer,//deviceLayers.data(),// nullptr,
            1, // uint32_t enabledExtensionCount
            &deviceExts[0], // const char * const *ppEnabledExtensionNames
            &features
        };
        //
        VkDevice deviceVK;
        //
        if (VK_SUCCESS == vkCreateDevice(m_deviceDescriptorVk.physicalDevice, &deviceCreateInfo, nullptr, &deviceVK)) {
            m_deviceDescriptorVk.queueFamilyCount = (uint32_t)deviceQueueCreateInfos.size();
            for( size_t i = 0; i<deviceQueueCreateInfos.size(); ++i) {
                m_deviceDescriptorVk.queueFamilyIndices[i] = deviceQueueCreateInfos[i].queueFamilyIndex;
            }
            //
            std::vector<VkQueue> graphicsQueue( m_deviceDescriptorVk.graphicsQueueCount );
            std::vector<VkQueue> transferQueue( m_deviceDescriptorVk.transferQueueCount );
            //
            std::vector<CommandQueue*> graphicsCommandQueues;
            std::vector<CommandQueue*> transferCommandQueues;
            //
            for( uint32_t i = 0; i < requestedGraphicsQueueItems.size(); ++i ) {
                auto& item = requestedGraphicsQueueItems[i];
                QueueItem queue = { item };
                vkGetDeviceQueue( deviceVK, queue.queueFamily, queue.queueIndex, &graphicsQueue[i]);
                //
                graphicsCommandQueues.push_back( new CommandQueue(graphicsQueue[i], queue.queueFamily, queue.queueIndex) );
            }
            for( uint32_t i = 0; i < requestedTransferQueueItems.size(); ++i ) {
                auto& item = requestedTransferQueueItems[i];
                QueueItem queue = { item };
                vkGetDeviceQueue( deviceVK, queue.queueFamily, queue.queueIndex, &transferQueue[i]);
                transferCommandQueues.push_back( new CommandQueue(transferQueue[i], queue.queueFamily, queue.queueIndex) );
            }
    
            VmaAllocatorCreateInfo vmaAllocatorInfo = {}; {
                vmaAllocatorInfo.device = deviceVK;
                vmaAllocatorInfo.physicalDevice = m_deviceDescriptorVk.physicalDevice;
                vmaAllocatorInfo.instance = m_deviceDescriptorVk.instance;
            }
            VmaAllocator vmaAllocator = nullptr;
            VkResult rst = vmaCreateAllocator( &vmaAllocatorInfo, &vmaAllocator);
            if( rst != VK_SUCCESS ) {
                return nullptr;
            }
            auto device = new Device( m_deviceDescriptorVk, deviceVK, vmaAllocator );
            device->_graphicsCommandQueues = std::move(graphicsCommandQueues);
            device->_transferCommandQueues = std::move(transferCommandQueues);
            // create render pass object manager
            device->_renderPassObjectManager = new RenderPassObjectManager();
            // create descriptor set allocator
            auto descriptorSetAllocator = new DescriptorSetAllocator();
            descriptorSetAllocator->initialize(deviceVK);
            device->_descriptorSetAllocator = descriptorSetAllocator;
            return device;
        }
        throw DeviceCreationException( DeviceCreationException::EXCEPTION_CREATE_DEVICE_FAILED);
        return nullptr;
    }

    void RenderSystem::createVulkanSurface() {
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
            (HWND)m_deviceDescriptorVk.wnd
        };
        rst = vkCreateWin32SurfaceKHR( m_deviceDescriptorVk.instance, &surface_create_info, nullptr, &surface );
#elif defined __ANDROID__
        VkAndroidSurfaceCreateInfoKHR surface_create_info = {
            VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,  // sType
            nullptr,// pNext
            0, // flag
            (ANativeWindow*)m_deviceDescriptorVk.wnd
        };
        rst = vkCreateAndroidSurfaceKHR(m_instance, &surface_create_info, nullptr, &surface);
#elif defined __linux__
        VkXlibSurfaceCreateInfoKHR surface_create_info = {
            VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,  // sType
            nullptr,// pNext
            0, // flag
            (Display*)m_deviceDescriptorVk.wnd
        };
        rst = vkCreateXlibSurfaceKHR(m_instance, &surface_create_info, nullptr, &surface);
#endif
        if (rst != VK_SUCCESS){
            throw DeviceCreationException( DeviceCreationException::EXCEPTION_CREATE_SURFACE_FAILED );
        }
        m_deviceDescriptorVk.surface = surface;
    }

    bool Device::acquireNextSwapchainImage( VkSwapchainKHR _swapchain, VkSemaphore _semaphore, uint32_t* _imageIndex ) {
        // we will not use fence
        VkResult rst = vkAcquireNextImageKHR( 
            _device,             // device
            _swapchain,         // swapchain
            ~0,                 // timeout
            _semaphore,         // semaphore to be signaled
            0,                  // fence to be signaled
            _imageIndex            // available image index
        );
        //
        switch (rst) {
        case VK_SUCCESS:
        case VK_SUBOPTIMAL_KHR:
            break;
        case VK_ERROR_OUT_OF_DATE_KHR:
            //updateSwapchain();
            // abort this frame, even you re create a swapchain right now, the `vkAcquireNextImageKHR` also will return a `VK_ERROR_OUT_OF_DATE_KHR` error code
        default:
            *_imageIndex = -1;
            return false;
        }
        return true;
    }

    Semaphore* Device::createSemaphore( uint32_t _semaphoreFlags ) {
        VkSemaphoreCreateInfo createInfo = {}; {
            createInfo.flags = _semaphoreFlags;
            createInfo.pNext = nullptr;
            createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        }
        VkSemaphore sem;
        VkResult rst = vkCreateSemaphore( _device, &createInfo, nullptr, &sem);
        if( rst == VK_SUCCESS){
            Semaphore* semaphore = new Semaphore(sem);
            return semaphore;
        }
        return nullptr;
    }

    Fence* Device::createFence( bool _signaled ) {
        VkFenceCreateInfo createInfo = {}; {
            createInfo.flags = _signaled ? 0x1 : 0x0;
            createInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            createInfo.pNext = nullptr;
        }
        VkFence fenceVk;
        VkResult rst = vkCreateFence( device(), &createInfo, nullptr, &fenceVk);
        if( VK_SUCCESS == rst ) {
            Fence* fence = new Fence(fenceVk);
            return fence;
        }
        return nullptr;
    }

    bool Device::isSignaled( const Fence* _fence ) {
        return Fence::IsSignaled( device(), *_fence);
    }

    Buffer* Device::createBuffer( BufferType _type, size_t _size ) {
        return Buffer::CreateBuffer( this, _type, _size );
    }

    Texture* Device::createTexture( const tex_desc_t& _desc, ResourceAccessType _accessType ) {
        return Texture::CreateTexture( this, 0, _desc, _accessType);
    }

    Swapchain* Device::createSwapchain( void* _wnd, AttachmentLoadAction loadAction ) {
        Swapchain* swapchain = new Swapchain();
        bool rst = swapchain->initialize( this, _wnd, loadAction );
        if( rst ) {
            return swapchain;
        }
        delete swapchain;
        return nullptr;
    }

    void Device::waitForFence( Fence* _fence ) {
        if( _fence->m_waitToSignaled ) {
            vkWaitForFences( device(), 1, &_fence->_fence, VK_TRUE, ~0 );
            _fence->m_waitToSignaled = false;
            vkResetFences( device(), 1, &_fence->_fence );
        }
    }

    GraphicsPipeline* Device::createGraphicsPipeline( const pipeline_desc_t& pipelineDescription ) {
        return GraphicsPipeline::CreatePipeline( this, pipelineDescription);
    }

    ComputePipeline* Device::createComputePipeline( const pipeline_desc_t& pipelineDescription ) {
        assert( pipelineDescription.shaders[(uint32_t)shader_type_t::ComputeShader].spirvData );
        return ComputePipeline::CreatePipeline(this, pipelineDescription);
    }

    Drawable* Device::createDrawable( const pipeline_desc_t& pipelineDescription ) {
        Drawable* drawable = new Drawable(pipelineDescription.vertexLayout.bufferCount);
        return drawable;
    }

    IRenderPass* Device::createRenderPass( const renderpass_desc_t& rpdesc, Texture** colors, Texture* ds, image_view_param_t const* colorViews, image_view_param_t dsView ) {
        return RenderPass::CreateRenderPass( this, rpdesc, colors, ds, colorViews, dsView);
    }

    UniformAllocator* Device::createUniformAllocator() {
        auto allocator = UniformAllocator::createUniformAllocator(this);
        return allocator;
    }

    void Device::destroyRenderPass( IRenderPass* renderPass ) {
        renderPass->release(this);
    }

    void Device::destroyTexture( Texture* texture ) {
        texture->release(this);
    }

    void Device::destroyBuffer( Buffer* buffer ) {
        buffer->release(this);
    }
}