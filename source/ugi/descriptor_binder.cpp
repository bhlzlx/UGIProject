#include "descriptor_binder.h"
#include "buffer.h"
#include "texture.h"
#include "descriptor_set_allocator.h"
#include "device.h"
#include "command_buffer.h"
#include "material_layout.inl"
#include <cstring>
#include <cstdint>

namespace ugi {

    VkSampler CreateSampler( Device* device, const sampler_state_t& samplerState );
    // 
    // vulkan 的 handle是 set/binding 组合
    // uint32_t DescriptorBinder::GetDescriptorHandle(const char* descriptorName, const pipeline_desc_t& pipelineDescription, res_descriptor_info_t* descriptorInfo ) 
    // {
    //     DescriptorHandleImp handle;
    //     handle.descriptorIndex = 0;
    //     handle.specifiedIndex = 0;
    //     handle.binding = 0;
    //     handle.handle = 0;
    //     //
    //     uint32_t dynamicBufferIndex = 0;
    //     uint32_t imageIndex = 0;

    //     for( uint32_t argIndex = 0; argIndex< pipelineDescription.argumentCount; ++argIndex) {
    //         auto setIndex = pipelineDescription.argumentLayouts[argIndex].index;
    //         handle.setID = setIndex;
    //         for( uint32_t descriptorIndex = 0; descriptorIndex < pipelineDescription.argumentLayouts[argIndex].descriptorCount; ++descriptorIndex) {
    //             const auto& descriptor = pipelineDescription.argumentLayouts[argIndex].descriptors[descriptorIndex];
    //             auto binding = descriptor.binding;
    //             assert(handle.binding <= binding);
    //             handle.binding = binding;
    //             if( strcmp(descriptor.name, descriptorName) == 0 ) {
    //                 handle.bindingIndex = descriptorIndex;                    
    //                 handle.setIndex = argIndex;
    //                 if(isDynamicBufferType( descriptor.type)) {
    //                     handle.specifiedIndex = dynamicBufferIndex;
    //                 } else if( isImageType( descriptor.type) ) {
    //                     handle.specifiedIndex = imageIndex;
    //                 }
    //                 assert( handle.specifiedIndex < 16 );
    //                 if(descriptorInfo) {
    //                     *descriptorInfo = descriptor;
    //                 }
    //                 return handle.handle;
    //             }
    //             ++handle.descriptorIndex;
    //             // 处理动态绑定（buffer）
    //             if(isDynamicBufferType( descriptor.type)) {
    //                 ++dynamicBufferIndex;
    //             } else if( isImageType( descriptor.type) ) {
    //                 ++imageIndex;
    //             }
    //         }
    //     }
    //     return ~0;
    // }

    void DescriptorBinder::_writeDescriptorResource( const res_descriptor_t& resource ) {
        DescriptorHandleImp h;
        h.handle = resource.handle;
        //
        auto& write = _descriptorWrites[h.descriptorIndex];
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.pNext = nullptr;
        write.dstSet = VK_NULL_HANDLE; // 为什么这里写nullptr? 因为写入 descriptor set 的时候才能确定是写入哪个 set
        write.dstBinding = h.binding;
        write.descriptorCount = 1;
        write.descriptorType = descriptorTypeToVk(resource.type);
        write.dstArrayElement = 0;
        // 这里有一个比较不好处理的地方就是这个 write.pBufferInfo pImageInfo
        auto& mixedDescriptor = _vecMixedDesciptorInfo[h.descriptorIndex];
        //
        switch (resource.type)
        {
        case res_descriptor_type::UniformBuffer: {
            VkDescriptorBufferInfo* pBufferInfo = (VkDescriptorBufferInfo*)&mixedDescriptor;
            if( pBufferInfo->buffer != (VkBuffer)resource.res.buffer.buffer) {
                pBufferInfo->buffer = (VkBuffer)resource.res.buffer.buffer;
                pBufferInfo->offset = 0; // resource.bufferOffset;  这个offset为什么是0？？因为绑定的时候还会再设置一次动态offset
                write.pBufferInfo = pBufferInfo;
            }
            pBufferInfo->range = resource.res.buffer.size;
            _dynamicOffsets[h.specifiedIndex] = resource.res.buffer.offset;
            break;
        }
        case res_descriptor_type::Image:{
            VkDescriptorImageInfo* pImageInfo = (VkDescriptorImageInfo*)&mixedDescriptor;
            VkImageView iv = (VkImageView)resource.res.imageView;
            if(pImageInfo->imageView != iv) {
                pImageInfo->imageView = iv;
                pImageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                pImageInfo->sampler = VK_NULL_HANDLE; // 再也不和sampler混合绑定了
                write.pImageInfo = pImageInfo;
            }
            break;
        }
        case res_descriptor_type::Sampler:{
            VkDescriptorImageInfo* pImageInfo = (VkDescriptorImageInfo*)&mixedDescriptor;
            pImageInfo->imageView = VK_NULL_HANDLE;
            pImageInfo->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            auto sampler = CreateSampler( _groupLayout->device(), resource.res.samplerState );
            if( pImageInfo->sampler != sampler ) {
                pImageInfo->sampler = sampler; // 再也不和 texture 混合绑定了
                write.pImageInfo = pImageInfo;
            }            
            break;
        }
        case res_descriptor_type::StorageImage: {
            VkDescriptorImageInfo* pImageInfo = (VkDescriptorImageInfo*)&mixedDescriptor;
            VkImageView iv = (VkImageView)resource.res.imageView;
            if( pImageInfo->imageView != iv) {
                pImageInfo->imageView = iv;
                pImageInfo->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
                pImageInfo->sampler = VK_NULL_HANDLE;
                write.pImageInfo = pImageInfo;
            }
            break;
        }
        default:
            assert(false);// 没支持的以后再支持
            break;
        }
        _reallocBitMask.set(h.setID);
    }

    DescriptorBinder::DescriptorBinder( const MaterialLayout* groupLayout, DescriptorSetAllocator* setAllocator, VkPipelineBindPoint bindPoint )
        : _groupLayout( groupLayout )
        , _resourceMasks {}
        , _resources {}
        , _vecMixedDesciptorInfo( groupLayout->descriptorCountTotal() )
        , _imageResources {}
        , _descriptorWrites( groupLayout->descriptorCountTotal() )
        , _dynamicOffsets( groupLayout->dynamicBufferCountTotal() )
        , _argumentBitMask( groupLayout->descriptorSetBitMask() )
        , _descriptorSets {}
        , _reallocBitMask (0)
        , _bindPoint(bindPoint)
        , _descriptorSetAllocator(setAllocator)
    {
        _reallocBitMask.flip();
    }

    bool DescriptorBinder::validateIntegrility() {
        return _groupLayout->validateResourceIntegrility(_resourceMasks);
    }

    void DescriptorBinder::updateDescriptor( const res_descriptor_t& resource ) {
        DescriptorHandleImp h;
        h.handle = resource.handle;
        uint32_t setIndex = h.setID;
        uint32_t binding = h.binding;

        if( resource.type == res_descriptor_type::Image || resource.type == res_descriptor_type::StorageImage) {
            _imageResources[h.specifiedIndex] = resource.res.imageView;
        }
        //
        _resourceMasks[setIndex] |= (1<<binding);
        _resources[setIndex][binding] = resource;
        //
        _writeDescriptorResource(resource);
    }

    bool DescriptorBinder::validateDescriptorSets() {
        if(!validateIntegrility()) {
            return false;
        }
        // 有必要就更新set
        for( uint32_t setID = 0; setID<_groupLayout->_descriptorSetCount; ++setID) {
            if( _reallocBitMask.test(setID) ) {
                _descriptorSets[setID] = _descriptorSetAllocator->allocate(_groupLayout->descriptorSetLayout(setID));
                VkDevice device = _groupLayout->device()->device();
                uint32_t descriptorCount = _groupLayout->descriptorCount(setID);
                uint32_t descriptorWriteBaseIndex = _groupLayout->descriptorWriteBaseIndex(setID);
                // 别忘了更新write里的set
                for( uint32_t i = descriptorWriteBaseIndex; i < (descriptorWriteBaseIndex + descriptorCount); ++i) {
                    _descriptorWrites[i].dstSet = _descriptorSets[setID];
                }
                vkUpdateDescriptorSets( 
                    device, 
                    descriptorCount, 
                    &_descriptorWrites[descriptorWriteBaseIndex], 
                    0,      // 不复制
                    nullptr
                );
                _reallocBitMask.flip(setID);
            }
        }
        return true;
    }

    void DescriptorBinder::bind( CommandBuffer const* commandBuffer ) {
        if(!validateDescriptorSets()) { // update descriptor set binding
            assert(false);
            return;
        }
        VkCommandBuffer cmdbuf = *commandBuffer;
        VkPipelineLayout pipelineLayout = _groupLayout->pipelineLayout();
        VkDescriptorSet desciprotSets[MaxArgumentCount];
        uint32_t descriptorSetCount = 0;
        for(uint32_t i = 0; i< MaxArgumentCount; ++i) {
            if(_descriptorSets[i]) {
                desciprotSets[descriptorSetCount++] = _descriptorSets[i];
            }
        }
        if(descriptorSetCount ) {
            if( _dynamicOffsets.size() ) {
                vkCmdBindDescriptorSets( cmdbuf, _bindPoint, pipelineLayout, 0, descriptorSetCount, desciprotSets, (uint32_t)_dynamicOffsets.size(), _dynamicOffsets.data() );
            } else {
                vkCmdBindDescriptorSets( cmdbuf, _bindPoint, pipelineLayout, 0, descriptorSetCount, desciprotSets, 0, nullptr );
            }
        }
    }

    VkPipelineLayout GetPipelineLayout( const MaterialLayout* argGroupLayout ) {
        return argGroupLayout->pipelineLayout();
    }

    // bool ArgumentGroup::prepairResource( ResourceCommandEncoder* commandEncoder ) {
    //     if( !validateDescriptorSets()) {
    //         return false;
    //     }
    //      // update all image layouts
    //     for( uint32_t i = 0; i < _groupLayout->imageResourceTotal(); ++i ) {
    //         auto type = _groupLayout->imageResourceType(i);
    //         if( type == ArgumentDescriptorType::Image ) {
    //             commandEncoder->imageTransitionBarrier( _imageResources[i].texture(), ResourceAccessType::ShaderRead, PipelineStages::Bottom, StageAccess::Write, PipelineStages::VertexInput, StageAccess::Read);
    //         } else if( type == ArgumentDescriptorType::StorageImage ) {
    //             commandEncoder->imageTransitionBarrier( _imageResources[i].texture(), ResourceAccessType::ShaderReadWrite, PipelineStages::Bottom, StageAccess::Write, PipelineStages::Top, StageAccess::Write);
    //         }
    //     }
    //     return true;
    // }

    void DescriptorBinder::reset() {
        _reallocBitMask.reset();
        _reallocBitMask.flip();
    }
    
    DescriptorBinder::~DescriptorBinder() {
    }
}


