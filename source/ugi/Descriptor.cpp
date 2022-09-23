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
        // , _freeTable {}
        // , _allocatedTable {}
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
        for(auto startIndex = _activePool; startIndex < _activePool+_vecDescriptorPool.size(); ++startIndex) {
            auto realIndex = startIndex % _vecDescriptorPool.size();
            auto pool = _vecDescriptorPool[realIndex];
            inf.descriptorPool = pool;
            auto rst = vkAllocateDescriptorSets(_device, &inf, &descriptorSet);
            if(rst == VK_SUCCESS) {
                _activePool = realIndex;
                return descriptorSet;
            }
        }
        // pool 不够了，新pool!
        auto pool = _createDescriporPool();
        _vecDescriptorPool.push_back(pool);
        inf.descriptorPool = pool;
        _activePool = _vecDescriptorPool.size() - 1;
        auto rst = vkAllocateDescriptorSets(_device, &inf, &descriptorSet);
        assert(rst == VK_SUCCESS);
        if( VK_SUCCESS != rst ) {
            return VK_NULL_HANDLE;
        }
        //
        AllocationInfo* info = nullptr;
        if(_allocationFlights[_flight].size()) {
            if(_allocationFlights[_flight].back().pool == _vecDescriptorPool[_activePool]) {
                info = &_allocationFlights[_flight].back();
            }
        }
        if(!info) {
            _vecDescriptorPool.emplace_back();
            info = &_allocationFlights[_flight].back();
            info->pool = _vecDescriptorPool[_activePool];
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
        _vecDescriptorPool.push_back(pool);
        _activePool = 0;
        return true;
    }

    void DescriptorSetAllocator::tick(uint32_t flight) {
        _flight = flight;
        auto& allocations = _allocationFlights[_flight];
        for(auto allocation: allocations) {
            vkFreeDescriptorSets(_device, allocation.pool, allocation.count, allocation.sets.data());
        }
        allocations.clear();
    }

    DescriptorSetAllocator* DescriptorSetAllocator::Instance() 
    {
        if( global_singleton_ptr == nullptr ) {
            global_singleton_ptr = new DescriptorSetAllocator();
        }
        return global_singleton_ptr;
    }
}