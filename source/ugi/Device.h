#pragma once

#include <cstdint>
#include <memory>
#include "VulkanFunctionDeclare.h"
#include <vk_mem_alloc.h>
#include <vector>
#include <unordered_map>

#include "UGIDeclare.h"
#include "UGITypes.h"

namespace hgl {
    namespace assets {
        class AssetsSource;
    }
}

namespace ugi {


    class ArgumentGroupLayout;
    struct DeviceDescriptorVulkan : public DeviceDescriptor {
        //
        constexpr static uint32_t MaxQueueCountSupport = 8;
        //
        VkInstance                              instance;
        VkPhysicalDeviceProperties              properties;
        VkPhysicalDevice                        physicalDevice;
        VkSurfaceKHR                            surface;
        hgl::assets::AssetsSource*              assetSource;
        //
        //
        uint32_t                                queueFamilyCount;
        uint32_t                                queueFamilyIndices[MaxQueueCountSupport];
        //
        DeviceDescriptorVulkan()
            : instance( nullptr )
            , properties( {} )
            , physicalDevice( nullptr )
            , surface ( 0 )
            , assetSource( nullptr ) {
        }
        
        DeviceDescriptorVulkan( const DeviceDescriptor& _baseDesc )
            : DeviceDescriptor( _baseDesc )
        {
        }
    };

    class Device {
        friend class RenderSystem;
    private:
        DeviceDescriptorVulkan              m_descriptor;
        VkDevice                            m_device;
        // std::vector<VkQueue>    m_graphicsQueues;
        // std::vector<VkQueue>    m_transferQueues;
        //      
        std::vector<CommandQueue*>          m_graphicsCommandQueues;
        std::vector<CommandQueue*>          m_transferCommandQueues;
        //      
        VmaAllocator                        m_vmaAllocator;
        
        RenderPassObjectManager*            m_renderPassObjectManager;
        DescriptorSetAllocator*             m_descriptorSetAllocator;

        // std::unordered_map<uint64_t, const ArgumentGroupLayout*>  m_argumentGroupLayoutCache;
        //
        Device() {
        }
        Device( const Device& ) {
        }
        //
        bool acquireNextSwapchainImage( VkSwapchainKHR _swapchain, VkSemaphore _semaphore, uint32_t* _imageIndex );
        //
    public:
        Device( const DeviceDescriptorVulkan& _descriptor, VkDevice _device, VmaAllocator _vmaAllocator = nullptr )
            : m_descriptor(_descriptor)
            , m_device( _device )
            , m_vmaAllocator( _vmaAllocator ) 
            , m_renderPassObjectManager( nullptr ) 
        {
        }

        VkPhysicalDevice physicalDevice() {
            return m_descriptor.physicalDevice;
        }
        VkDevice device() {
            return m_device;
        }
        VkInstance instance() {
            return m_descriptor.instance;
        }
        VmaAllocator vmaAllocator() {
            return m_vmaAllocator;
        }
        const DeviceDescriptorVulkan& descriptor() {
            return m_descriptor;
        }

        RenderPassObjectManager* renderPassObjectManager() {
            return m_renderPassObjectManager;
        }

        const std::vector<CommandQueue*>& graphicsQueues() {
            return m_graphicsCommandQueues;
        }

        const std::vector<CommandQueue*>& transferQueues() {
            return m_transferCommandQueues;
        }

        Semaphore* createSemaphore( uint32_t _semaphoreFlags = 0);
        Fence* createFence( bool _signaled = false );
        bool isSignaled( const Fence* _fence );
        Buffer* createBuffer( BufferType _type, size_t _size );
        Texture* createTexture( const TextureDescription& _desc, ResourceAccessType _accessType = ResourceAccessType::ShaderReadWrite );
        IRenderPass* createRenderPass( const RenderPassDescription& _renderPass, Texture** _colors, Texture* _depthStencil );
        Swapchain* createSwapchain( void* _wnd );
        Pipeline* createPipeline( const PipelineDescription& pipelineDescription );
        Drawable* createDrawable( const PipelineDescription& pipelineDescription );
        UniformAllocator* createUniformAllocator();

        void destroyRenderPass( IRenderPass* renderPass );
        void destroyTexture( Texture* texture );
        void destroyBuffer( Buffer* texture );

        ///---
        void registArgumentGroupLayout( const PipelineDescription& pipelineDescription, const ArgumentGroupLayout* argumentGroupLayout );
        const ArgumentGroupLayout* getArgumentGroupLayout( const PipelineDescription& pipelineDescription, uint64_t* hashPtr );
        const ArgumentGroupLayout* getArgumentGroupLayout( uint64_t hash ) const;
        ///> --------------- Hash Function For Graphics Objects ------------------
        template< class T >
        uint64_t hashObject();
        ///> --------------- Hash Function For Graphics Objects ------------------
        void waitForFence( Fence* _fence );
    public:

    };

    class DeviceCreationException {
    public:
        enum ExceptionType {
            EXCEPTION_NONE = 0,
            EXCEPTION_LOAD_PROC_ADDRESS_FAILED = 0xff00,
            EXCEPTION_NO_PHYSICAL_DEVICE_AVAILABLE = 0xff01,
            EXCEPTION_NO_QUEUE_FAMILY_AVAILABLE = 0xff02,
            EXCEPTION_CREATE_SURFACE_FAILED = 0xff03,
            EXCEPTION_CREATE_DEVICE_FAILED = 0xff04,
        };
    private:
        ExceptionType m_exceptionType;
    public:

        DeviceCreationException( ExceptionType _type = ExceptionType::EXCEPTION_NONE )
            : m_exceptionType ( _type )
        {
        }
    };

    class RenderSystem {
    private:
        DeviceDescriptorVulkan m_deviceDescriptorVk;
        //
        void createVulkanInstance( bool validation = false );
        void selectVulkanPhysicalDevice();
        void createVulkanSurface();
        Device* createVulkanDevice();
    public:
        RenderSystem() {
        }
        Device*
        createDevice( const DeviceDescriptor& _descriptor, hgl::assets::AssetsSource* assetSource );
        //
        const DeviceDescriptorVulkan* getVulkanDeviceDescriptor() const {
            return &m_deviceDescriptorVk;
        }
    };

}

