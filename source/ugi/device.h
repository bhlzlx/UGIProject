#pragma once

#include <cstdint>
#include <memory>
#include "vulkan_function_declare.h"
#include <vk_mem_alloc.h>
#include <vector>
#include <unordered_map>

#include "ugi_declare.h"
#include "ugi_types.h"
#include <LightWeightCommon/io/archive.h>
#include <flight_cycle_invoker.h>

namespace ugi {

    class MaterialLayout;
    struct DeviceDescriptorVulkan : public device_descriptor_t {
        //
        constexpr static uint32_t MaxQueueCountSupport = 8;
        //
        VkInstance                              instance;
        VkPhysicalDeviceProperties              properties;
        VkPhysicalDevice                        physicalDevice;
        VkSurfaceKHR                            surface;
        comm::IArchive*                         archive;
        //
        //
        uint32_t                                queueFamilyCount;
        uint32_t                                queueFamilyIndices[MaxQueueCountSupport];
        //
        DeviceDescriptorVulkan()
            : instance(nullptr)
            , properties({})
            , physicalDevice(nullptr)
            , surface (0)
            , archive(nullptr) {
        }
        
        DeviceDescriptorVulkan( const device_descriptor_t& _baseDesc )
            : device_descriptor_t( _baseDesc )
        {
        }
    };

    class Device {
        friend class RenderSystem;
    private:
        DeviceDescriptorVulkan              _descriptor;
        VkDevice                            _device;
        std::vector<CommandQueue*>          _graphicsCommandQueues;
        std::vector<CommandQueue*>          _transferCommandQueues;
        //      
        VmaAllocator                        _vmaAllocator;
        
        RenderPassObjectManager*            _renderPassObjectManager;
        DescriptorSetAllocator*             _descriptorSetAllocator;
        FlightCycleInvoker                  _cycleInvoker;
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
            : _descriptor(_descriptor)
            , _device( _device )
            , _vmaAllocator( _vmaAllocator ) 
            , _renderPassObjectManager( nullptr ) 
        {
        }

        VkPhysicalDevice physicalDevice() {
            return _descriptor.physicalDevice;
        }
        VkDevice device() {
            return _device;
        }
        VkInstance instance() {
            return _descriptor.instance;
        }
        VmaAllocator vmaAllocator() const {
            return _vmaAllocator;
        }
        const DeviceDescriptorVulkan& descriptor() const {
            return _descriptor;
        }

        RenderPassObjectManager* renderPassObjectManager() const {
            return _renderPassObjectManager;
        }

        const std::vector<CommandQueue*>& graphicsQueues() const {
            return _graphicsCommandQueues;
        }

        const std::vector<CommandQueue*>& transferQueues() const {
            return _transferCommandQueues;
        }

        FlightCycleInvoker& cycleInvoker() {
            return _cycleInvoker;
        }

        Semaphore* createSemaphore( uint32_t _semaphoreFlags = 0);
        Fence* createFence( bool _signaled = false );
        bool isSignaled( const Fence* _fence );
        Buffer* createBuffer( BufferType _type, size_t _size );
        Texture* createTexture( const tex_desc_t& _desc, ResourceAccessType _accessType = ResourceAccessType::ShaderReadWrite );
        IRenderPass* createRenderPass( const renderpass_desc_t& _renderPass, Texture** colors, Texture* ds, image_view_param_t const* colorViews, image_view_param_t dsView );
        Swapchain* createSwapchain( void* wnd, AttachmentLoadAction loadAction = AttachmentLoadAction::Clear );
        GraphicsPipeline* createGraphicsPipeline( const pipeline_desc_t& pipelineDescription );
        ComputePipeline* createComputePipeline( const pipeline_desc_t& pipelineDescription );
        UniformAllocator* createUniformAllocator();
        // DescriptorSetAllocator* createDescriptorSetAllocator() const;

        void destroyRenderPass( IRenderPass* renderPass );
        void destroyTexture( Texture* texture );
        void destroyBuffer( Buffer* texture );
        void destroyFence( Fence* fence );

        const MaterialLayout* getPipelineMaterialLayout( const pipeline_desc_t& pipelineDescription, uint64_t& hashval );
        const MaterialLayout* getPipelineMaterialLayout( uint64_t hashval );
        DescriptorSetAllocator* descriptorSetAllocator() const {
            return _descriptorSetAllocator;
        }
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
        Device* createDevice( const device_descriptor_t& _descriptor, comm::IArchive* archive);
        //
        const DeviceDescriptorVulkan* getVulkanDeviceDescriptor() const {
            return &m_deviceDescriptorVk;
        }
    };

}

