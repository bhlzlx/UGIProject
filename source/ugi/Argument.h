#pragma once
#include "UGIDeclare.h"
#include "UGITypes.h"
#include "VulkanFunctionDeclare.h"
#include <vector>
#include <cassert>
#include <cstdlib>
#include <memory.h>
#include <map>

namespace ugi {
    //
    class ArgumentGroup {
    private:
        const ArgumentGroupLayout*                                      _groupLayout;
        // 资源绑定信息最终决定放在ArgumentGroup对象里了，主要是因为这个uniform更新的时候如果是做ringbuffer那么这个buffer在下一次绑定的时候可能就不是它了，这个情况下需要重新生成新的 descriptor,旧的 descriptor 回收
        // 所以我们如果需要做这个判断，就需要存储之前的绑定信息，所以最终决定绑定的资源放在这里了
        uint32_t                                                        _resourceMasks[MaxArgumentCount];   ///> 记录哪些资源已经绑定上了，资源刷新绑定到 descriptor set上时是需要做完整性检验的
        std::map< uint32_t, std::map<uint32_t, ResourceDescriptor> >    _resources;                         ///> 这个结构先放在这，以后观察看看还需要不需要它
        //
        struct MixedDescriptorInfo {
            union {
                VkDescriptorBufferInfo      bufferInfo;
                VkDescriptorImageInfo       imageInfo;
                VkBufferView                bufferView;
            };
        }; // 24 bytes
        std::vector<MixedDescriptorInfo>                                _vecMixedDesciptorInfo;
        std::vector<VkWriteDescriptorSet>                               _descriptorWrites;
        //VkWriteDescriptorSet                                            _descriptorWrites[MaxDescriptorCount][MaxArgumentCount]; ///> vulkan 的刷新 descriptor set的结构， 这里实际上还能再省内存
        std::vector<uint32_t>                                           _dynamicOffsets;
        //
        uint32_t                                                        _argumentBitMask;
        VkDescriptorSet                                                 _descriptorSets[MaxArgumentCount];
        //
        ImageView                                                       _imageResources[16];
        //
        uint32_t                                                        _reallocDescriptorSetBits;      ///> 某些descriptr绑定改变就需要更新/更换 descriptor set
        VkPipelineBindPoint                                             _bindPoint;
    private:
        void _writeDescriptorResource( const ResourceDescriptor& resource );
        // 看 descriptor set 需要不需要重建，如果需要就重建 descriptor set，回收旧的
        bool validateIntegrility();
        bool validateDescriptorSets();
    public:
        ArgumentGroup( const ArgumentGroupLayout* groupLayout, VkPipelineBindPoint bindPoint = VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS );
        //* 更新绑定的 API
        void updateDescriptor( const ResourceDescriptor& resource );
        bool prepairResource( ResourceCommandEncoder* encoder );
        bool prepairResource();
        void bind( CommandBuffer* commandBuffer );
        ~ArgumentGroup();
    public:
        static uint32_t GetDescriptorHandle( const char* descriptorName, const PipelineDescription& pipelineDescription );
    };

}