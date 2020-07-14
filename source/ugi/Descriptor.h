#pragma once

#include "VulkanDeclare.h"
#include "UGITypes.h"
#include <map>
#include <vector>
#include <list>
#include <cassert>

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
    private:
        VkDevice                                                        _device;
        std::map<VkDescriptorSetLayout, std::list<VkDescriptorSet>>     _freeTable;
        std::map<VkDescriptorSet, VkDescriptorSetLayout>                _allocatedTable;
        std::vector<VkDescriptorPool>                                   _vecDescriptorPool;
        //
        static DescriptorSetAllocator* global_singleton_ptr;
    private:
        VkDescriptorPool _createDescriporPool();
        DescriptorSetAllocator();
    public:        
        bool initialize( VkDevice device );
        VkDescriptorSet allocate( VkDescriptorSetLayout setLayout );
        void free( VkDescriptorSet descriptorSet );
        //
        static DescriptorSetAllocator* Instance();
    };

}