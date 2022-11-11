/**
 * @file Argument.h
 * @author lixin
 * @brief 
 * @version 0.1
 * @date 2022-09-28
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once
#include "UGIDeclare.h"
#include "UGIVulkanPrivate.h"
#include "VulkanFunctionDeclare.h"
#include <vector>
#include <cassert>
#include <cstdlib>
#include <memory.h>
#include <map>
#include <bitset>

namespace ugi {
    //
    class DescriptorBinder {
    private:
        const MaterialLayout*                                      _groupLayout;
        // 资源绑定信息最终决定放在ArgumentGroup对象里了，主要是因为这个uniform更新的时候如果是做ringbuffer那么这个buffer在下一次绑定的时候可能就不是它了，这个情况下需要重新生成新的 descriptor,旧的 descriptor 回收
        // 所以我们如果需要做这个判断，就需要存储之前的绑定信息，所以最终决定绑定的资源放在这里了
        uint32_t                                                        _resourceMasks[MaxArgumentCount];   ///> 记录哪些资源已经绑定上了，资源刷新绑定到 descriptor set上时是需要做完整性检验的
        std::map< uint32_t, std::map<uint32_t, res_descriptor_t> >      _resources;                         ///> 这个结构先放在这，以后观察看看还需要不需要它
        //
        struct MixedDescriptorInfo {
            union {
                VkDescriptorBufferInfo      bufferInfo;
                VkDescriptorImageInfo       imageInfo;
                VkBufferView                bufferView;
            };
        }; // 24 bytes
        std::vector<MixedDescriptorInfo>                                _vecMixedDesciptorInfo;
        size_t                                                          _imageResources[16];
        std::vector<VkWriteDescriptorSet>                               _descriptorWrites;
        std::vector<uint32_t>                                           _dynamicOffsets;
        //
        uint32_t                                                        _argumentBitMask;
        VkDescriptorSet                                                 _descriptorSets[MaxArgumentCount];
        std::bitset<MaxArgumentCount>                                   _reallocBitMask;      ///> 某些descriptr绑定改变就需要更新/更换 descriptor set
        VkPipelineBindPoint                                             _bindPoint;
        DescriptorSetAllocator*                                         _descriptorSetAllocator;
    private:
        void _writeDescriptorResource( const res_descriptor_t& resource );
        // 看 descriptor set 需要不需要重建，如果需要就重建 descriptor set，回收旧的
        bool validateIntegrility();
        bool validateDescriptorSets();
    public:
        DescriptorBinder(const MaterialLayout* groupLayout, DescriptorSetAllocator* setAllocator, VkPipelineBindPoint bindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS );
        void reset();
        //* 更新绑定的 API
        void updateDescriptor(const res_descriptor_t& resource);
        // bool prepairResource(ResourceCommandEncoder* encoder);
        void bind(CommandBuffer const* commandBuffer);
        ~DescriptorBinder();
    public:
        // static uint32_t GetDescriptorHandle(const char* descriptorName, const pipeline_desc_t& pipelineDescription, res_descriptor_info_t* descriptorInfo = nullptr);
    };

}