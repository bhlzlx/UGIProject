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

    DescriptorSetAllocator::DescriptorSetAllocator()
        : _device( nullptr )
        , _activePool(0)
        , _flight(0)         
        , _descriptorPools {}
        , _allocationFlights{}
    {}

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

    VkDescriptorSet DescriptorSetAllocator::allocate(VkDescriptorSetLayout setLayout) 
    {
        VkDescriptorSetAllocateInfo inf = {}; {
            inf.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            inf.pNext = nullptr;
            inf.descriptorSetCount = 1;
            inf.pSetLayouts = &setLayout;
        }
        VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
        // 遍历现有pool，去试着分配
        for(auto startIndex = _activePool; startIndex < _activePool+_descriptorPools.size(); ++startIndex) {
            auto realIndex = startIndex % _descriptorPools.size();
            auto pool = _descriptorPools[realIndex];
            inf.descriptorPool = pool;
            auto rst = vkAllocateDescriptorSets(_device, &inf, &descriptorSet);
            if(rst == VK_SUCCESS) {
                _activePool = realIndex;
                return descriptorSet;
            }
        }
        // pool 不够了，新pool!
        auto pool = _createDescriporPool();
        _descriptorPools.push_back(pool);
        inf.descriptorPool = pool;
        _activePool = _descriptorPools.size() - 1;
        auto rst = vkAllocateDescriptorSets(_device, &inf, &descriptorSet);
        assert(rst == VK_SUCCESS);
        if( VK_SUCCESS != rst ) {
            return VK_NULL_HANDLE;
        }
        //
        AllocationInfo* info = nullptr;
        if(_allocationFlights[_flight].size()) {
            if(_allocationFlights[_flight].back().pool == _descriptorPools[_activePool]) {
                info = &_allocationFlights[_flight].back();
            }
        }
        if(!info) {
            _descriptorPools.emplace_back();
            info = &_allocationFlights[_flight].back();
            info->pool = _descriptorPools[_activePool];
        }
        info->sets.push_back(descriptorSet);
        ++info->count;
        return descriptorSet;
    }

    bool DescriptorSetAllocator::initialize( VkDevice device ) 
    {
        _device = device;
        auto pool = _createDescriporPool();
        if( !pool ) {
            return false;
        }
        _descriptorPools.push_back(pool);
        _activePool = 0;
        _flight = 0;
        return true;
    }

    void DescriptorSetAllocator::tick() {
        _flight++;
        _flight %= MaxFlightCount;
        auto& allocations = _allocationFlights[_flight];
        for(auto allocation: allocations) {
            vkFreeDescriptorSets(_device, allocation.pool, allocation.count, allocation.sets.data());
        }
        allocations.clear();
    }

    void DescriptorSetAllocator::destroy() {
        for( auto pool : _descriptorPools ) {
            vkDestroyDescriptorPool(_device, pool, nullptr);
        }
    }

}