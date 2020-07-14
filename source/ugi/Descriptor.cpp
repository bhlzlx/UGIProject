#include "Descriptor.h"
#include "VulkanFunctionDeclare.h"
namespace ugi {

    constexpr VkDescriptorPoolSize DescriptorPoolSizeTemplate[] = {
		{ VK_DESCRIPTOR_TYPE_SAMPLER                    , 512 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER     , 512 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE              , 512 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE              , 512 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER       , 512 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER       , 512 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER             , 512 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER             , 512 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC     , 512 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC     , 512 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT           , 512 },
	};

    DescriptorSetAllocator* DescriptorSetAllocator::global_singleton_ptr = nullptr;

    DescriptorSetAllocator::DescriptorSetAllocator()
        : _device( nullptr )
        , _freeTable {}
        , _allocatedTable {}
        , _vecDescriptorPool {}
    {
    }

    VkDescriptorPool DescriptorSetAllocator::_createDescriporPool() 
    {
        VkDescriptorPoolCreateInfo descriptorPoolInfo = {}; {
            descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            descriptorPoolInfo.pNext = nullptr;
            descriptorPoolInfo.poolSizeCount = (uint32_t) sizeof(DescriptorPoolSizeTemplate)/sizeof(VkDescriptorPoolSize);
            descriptorPoolInfo.pPoolSizes = DescriptorPoolSizeTemplate;
            descriptorPoolInfo.maxSets = 2048;
        }
        VkDescriptorPool pool;
        VkResult rst = vkCreateDescriptorPool( _device, &descriptorPoolInfo, nullptr, &pool);
        if( rst == VK_SUCCESS ) {
            return pool;
        }
        return VK_NULL_HANDLE;
    }

    VkDescriptorSet DescriptorSetAllocator::allocate( VkDescriptorSetLayout setLayout ) 
    {
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        // 从缓存里查找
        auto iter = _freeTable.find(setLayout);
        if( iter != _freeTable.end() && iter->second.size() ) {
            descriptorSet = iter->second.back();
            iter->second.pop_back();
        } else {
            // 从pool里分配
            auto pool = _vecDescriptorPool.back();
            VkDescriptorSetAllocateInfo inf = {}; {
                inf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
                inf.pNext = nullptr;
                inf.descriptorPool = pool;
                inf.descriptorSetCount = 1;
                inf.pSetLayouts = &setLayout;
            }
            auto rst = vkAllocateDescriptorSets( _device, &inf, &descriptorSet);
            if( rst != VK_SUCCESS) {
                // pool 不够了，新pool!
                pool = _createDescriporPool();
                _vecDescriptorPool.push_back(pool);
                rst = vkAllocateDescriptorSets( _device, &inf, &descriptorSet);
                assert(rst == VK_SUCCESS);
                if( VK_SUCCESS != rst ) {
                    return VK_NULL_HANDLE;
                }
            }
        }
        // 添加分配记录
        _allocatedTable[descriptorSet] = setLayout;
        return descriptorSet;
    }

    void DescriptorSetAllocator::free( VkDescriptorSet descriptorSet ) 
    {
        auto iter = _allocatedTable.find(descriptorSet);
        assert(iter != _allocatedTable.end());
        if( iter == _allocatedTable.end()) {
            return;
        }
        _freeTable[iter->second].push_back(iter->first);
        _allocatedTable.erase(iter);
    }

    bool DescriptorSetAllocator::initialize( VkDevice device ) 
    {
        _device = device;
        auto pool = _createDescriporPool();
        if( !pool ) {
            return false;
        }
        _vecDescriptorPool.push_back(pool);
        return true;
    }

    DescriptorSetAllocator* DescriptorSetAllocator::Instance() 
    {
        if( global_singleton_ptr == nullptr ) {
            global_singleton_ptr = new DescriptorSetAllocator();
        }
        return global_singleton_ptr;
    }
}