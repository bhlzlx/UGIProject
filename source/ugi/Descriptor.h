#pragma once

#include "VulkanDeclare.h"
#include "UGITypes.h"
#include <map>
#include <vector>
#include <list>
#include <cassert>
#include <array>

namespace ugi {

    /*=================================================================
        与 DX12 不同，Vulkan 分配 descriptor 是以 set 为单位的
        descriptor pool -> descriptor set -> bind resources
        在 vk 里 argument 对应一个 descriptor set
    =================================================================*/

    /*****************************************************************************************************************
    *  类描述 ：descriptor set allocator
    *  这个类负责分配descriptor set，原则上只分配不回收，即只从 VkDescriptorPool 中分配，不回收到VkDescriptorPool中
    *  但我们会提供一个缓存机制代替回收功能
    *  所以 m_vecDescriptorPool 的内容只会增加不会减少，这一点务必注意
    *******************************************************************************************************************/
    class DescriptorSetAllocator {
        struct AllocationInfo { // 记录一个pool里有多个set，分别是什么，方便以后回收
            VkDescriptorPool                pool;
            uint32_t                        count;
            std::vector<VkDescriptorSet>    sets;
        };
    private:
        VkDevice                                                        _device;
        uint32_t                                                        _activePool;
        uint32_t                                                        _flight;         
        std::vector<VkDescriptorPool>                                   _descriptorPools;
        std::array<std::vector<AllocationInfo>, MaxFlightCount>         _allocationFlights;
    private:
        VkDescriptorPool _createDescriporPool();
    public:        
        DescriptorSetAllocator();
        bool initialize( VkDevice device );
        void tick();
        VkDescriptorSet allocate(VkDescriptorSetLayout setLayout);
        void destroy();
    };

}