#include "Argument.h"
#include "Buffer.h"
#include "Texture.h"
#include "Descriptor.h"
#include "Device.h"
#include <cstring>
#include "CommandBuffer.h"
#include "UGIUtility.h"


namespace ugi {

    VkSampler CreateSampler( VkDevice device, const SamplerState& samplerState );

    inline bool isDynamicBufferType( ArgumentDescriptorType type ) {
        if( type == ArgumentDescriptorType::UniformBuffer ) {
            return true;
        }
        return false;
    }

    inline bool isImageType( ArgumentDescriptorType type ) {
        if( type == ArgumentDescriptorType::Image || type == ArgumentDescriptorType::StorageImage ) {
            return true;
        }
        return false;
    }
    // 
    struct DescriptorHandleImp {
        union {
            struct {
                uint32_t setID          : 2;        // set id 最多4个
                uint32_t setIndex       : 2;        // set index 最多4个
                uint32_t binding        : 5;        // descriptor 最多支持32个，够了吧！
                uint32_t bindingIndex   : 5;        // desctiptor 有时候不是按binding顺序排列的 用来标记这个排列的位置
                uint32_t descriptorIndex: 8;        // 整个 argument group 里的 索引
                uint32_t specifiedIndex : 4;        // 支持16个，够了吧！16 个 dynamic buffer / image / or other specified type
            };
            uint32_t handle;
        };
    };
    
    // vulkan 的 handle是 set/binding 组合
    uint32_t ArgumentGroup::GetDescriptorHandle( const char* descriptorName, const PipelineDescription& pipelineDescription ) 
    {
        DescriptorHandleImp handle;
        handle.descriptorIndex = 0;
        handle.specifiedIndex = 0;
        handle.binding = 0;
        //
        uint32_t dynamicBufferIndex = 0;
        uint32_t imageIndex = 0;

        for( uint32_t argIndex = 0; argIndex< pipelineDescription.argumentCount; ++argIndex) {
            auto setIndex = pipelineDescription.argumentLayouts[argIndex].index;
            handle.setID = setIndex;
            for( uint32_t descriptorIndex = 0; descriptorIndex < pipelineDescription.argumentLayouts[argIndex].descriptorCount; ++descriptorIndex) {
                const auto& descriptor = pipelineDescription.argumentLayouts[argIndex].descriptors[descriptorIndex];
                auto binding = descriptor.binding;
                assert(handle.binding <= binding);
                handle.binding = binding;
                if( strcmp(descriptor.name, descriptorName) == 0 ) {
                    handle.bindingIndex = descriptorIndex;                    
                    handle.setIndex = argIndex;
                    if(isDynamicBufferType( descriptor.type)) {
                        handle.specifiedIndex = dynamicBufferIndex;
                    } else if( isImageType( descriptor.type) ) {
                        handle.specifiedIndex = imageIndex;
                    }
                    assert( handle.specifiedIndex < 16 );
                    return handle.handle;
                }
                ++handle.descriptorIndex;
                // 处理动态绑定（buffer）
                if(isDynamicBufferType( descriptor.type)) {
                    ++dynamicBufferIndex;
                } else if( isImageType( descriptor.type) ) {
                    ++imageIndex;
                }
            }
        }
        return ~0;
    }

    

    /* ===============================================================================================================
    *   Argument Group Layout
    *   只存参数的Layout信息
    * ==============================================================================================================*/
    class ArgumentGroupLayout
    {
    private:
        Device*                 _device;
        // descriptor set
        uint32_t                _descriptorSetCount;                        ///> descriptor set 数量
        uint32_t                _descriptorSetBitMask;                      ///> 用来确定有哪些 descriptor set layout
        VkDescriptorSetLayout   _descriptorSetLayouts[MaxArgumentCount];
        // descriptors
        uint32_t                _descriptorCountTotal;                      ///> 所有的 descriptor 数量
        uint32_t                _descriptorBindingMasks[MaxArgumentCount];  ///> descriptor 全部绑定好的情况
        uint32_t                _descriptorCount[MaxArgumentCount];         ///> 每个 set 有几个 descriptor ?
        uint32_t                _descriptorSetWriteBaseIndex[MaxArgumentCount]; ///> argument里有很多 VkWriteDescriptorSet对象，每个set占用若干个，这里记录set对应的起始index
        // 特殊用途
        uint32_t                _dynamicBufferCountTotal;                   ///> 比如说 uniform buffer 这种动态偏移绑定的
        uint32_t                _dynamicBufferCount[MaxArgumentCount];      ///> 每个set有多少个 dynamic descriptor
        uint32_t                _dynamicOffsetBaseIndex[MaxArgumentCount];  ///> 把所有的dynamic offset存数组里，每个set的 起始 dynamic descriptor 对应的索引
        //
        uint32_t                _imageCountTotal;
        ArgumentDescriptorType  _imageDescriptorType[MaxDescriptorCount];   ///> 这个量是随便写的，不过一个管线一般也不会超过8张纹理吧！
        //
        VkPipelineLayout        _pipelineLayout;
    public:
        ArgumentGroupLayout();
        // ===============================================================================================================
        ArgumentGroupLayout( ArgumentGroupLayout&& groupLayout );
        //
        bool validateResourceIntegrility( uint32_t _mask[] ) const;
        //
        uint32_t descriptorCountTotal() const {
            return _descriptorCountTotal;
        }
        uint32_t dynamicBufferCountTotal() const {
            return _dynamicBufferCountTotal;
        }
        uint32_t dynamicBufferCount( uint32_t setID ) const {
            return _dynamicBufferCount[setID];
        }
        uint32_t dynamicOffsetBaseIndex( uint32_t setID) const {
            return _dynamicOffsetBaseIndex[setID];
        }
        uint32_t descriptorSetBitMask() const {
            return _descriptorSetBitMask;
        }
        uint32_t descriptorCount( uint32_t setID ) const {
            return _descriptorCount[setID];
        }
        uint32_t descriptorWriteBaseIndex( uint32_t setID) const {
            return _descriptorSetWriteBaseIndex[setID];
        }
        uint32_t imageResourceTotal() const {
            return _imageCountTotal;
        }
        ArgumentDescriptorType imageResourceType( uint32_t idx ) const {
            return _imageDescriptorType[idx];
        }
        VkDescriptorSetLayout descriptorSetLayout( uint32_t setID ) const {
            return _descriptorSetLayouts[setID];
        }
        VkPipelineLayout pipelineLayout() const {
            return _pipelineLayout;
        }
        Device* device() const {
            return _device;
        }
        // ===============================================================================================================
        static ArgumentGroupLayout* CreateArgumentGroupLayout( Device* device, const PipelineDescription& pipelineDescription );
    };

    ArgumentGroupLayout::ArgumentGroupLayout()
        : _device( nullptr )
        , _descriptorSetCount(0)
        , _descriptorSetBitMask {}
        , _descriptorSetLayouts {}
        , _descriptorCountTotal(0)
        , _descriptorBindingMasks {}
        , _descriptorCount {}
        , _descriptorSetWriteBaseIndex {}
        , _dynamicBufferCountTotal(0)
        , _dynamicBufferCount {}
        , _dynamicOffsetBaseIndex {}
        , _imageCountTotal(0)
        , _imageDescriptorType {}
    {
    }

    bool ArgumentGroupLayout::validateResourceIntegrility( uint32_t _mask[] ) const {
        int rst = memcmp( _descriptorBindingMasks, _mask, sizeof(_descriptorBindingMasks));
        if( rst == 0 ) {
            return true;
        }
        return false;
    }
    // ================================================================================================================================
    ArgumentGroupLayout::ArgumentGroupLayout( ArgumentGroupLayout&& groupLayout ) 
    {
        _descriptorSetCount = groupLayout._descriptorSetCount;
        memcpy(_descriptorBindingMasks,groupLayout._descriptorBindingMasks, sizeof(_descriptorBindingMasks));
        memcpy(_descriptorSetLayouts, groupLayout._descriptorSetLayouts, sizeof(VkDescriptorSetLayout) * _descriptorSetCount) ;
        groupLayout._descriptorSetCount = 0;
        memset(groupLayout._descriptorBindingMasks, 0, sizeof(groupLayout._descriptorBindingMasks));
        memset(groupLayout._descriptorSetLayouts, 0, sizeof(VkDescriptorSetLayout) * _descriptorSetCount);
    }
    
    // ================================================================================================================================
    ArgumentGroupLayout* ArgumentGroupLayout::CreateArgumentGroupLayout( Device* device, const PipelineDescription& pipelineDescription ) {
        // 实际上这个 ArgumentGroupLayout 也应该作为一个缓存资源，因为很多 argument groupt 会共享这个对象，所以也应该做一个哈希来存这个玩意
        // 目前会写到device的数据成员里（缓存
        ArgumentGroupLayout* layoutPtr = new ArgumentGroupLayout();
        ArgumentGroupLayout& groupLayout = *layoutPtr;
        groupLayout._device = device;
        groupLayout._descriptorSetCount = pipelineDescription.argumentCount;
        //
        for( uint32_t argIndex = 0; argIndex< pipelineDescription.argumentCount; ++argIndex) {
            uint32_t setID = pipelineDescription.argumentLayouts[argIndex].index;
            groupLayout._descriptorSetBitMask |= 1 << setID;
            VkDescriptorSetLayoutBinding bindings[MaxDescriptorCount];
            VkDescriptorSetLayoutCreateInfo layoutInfo = {}; {
                layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
                layoutInfo.pNext = nullptr;
                layoutInfo.flags = 0;
                layoutInfo.bindingCount = pipelineDescription.argumentLayouts[argIndex].descriptorCount;
                layoutInfo.pBindings = bindings;
            }
            //
            groupLayout._descriptorCount[setID] = pipelineDescription.argumentLayouts[argIndex].descriptorCount;  
            //
            groupLayout._dynamicOffsetBaseIndex[setID] = groupLayout._dynamicBufferCountTotal;
            groupLayout._descriptorSetWriteBaseIndex[setID] = groupLayout._descriptorCountTotal;
            ///> 
            for( uint32_t descIndex = 0; descIndex<layoutInfo.bindingCount; ++descIndex) {
                auto& universalDescriptor = pipelineDescription.argumentLayouts[argIndex].descriptors[descIndex];
                auto& nativeDescriptor = bindings[descIndex];

                nativeDescriptor.binding = universalDescriptor.binding;
                nativeDescriptor.descriptorCount = 1;
                nativeDescriptor.pImmutableSamplers = nullptr;
                //
                nativeDescriptor.stageFlags = stageFlagsToVk(universalDescriptor.shaderStage);
                nativeDescriptor.descriptorType = descriptorTypeToVk(universalDescriptor.type);
                //
                ++groupLayout._descriptorCountTotal; // 用了多少descriptor来个计数！！方便之后更新的时候提供额外的pBufferInfo的时候省内存
                if( isDynamicBufferType(universalDescriptor.type)) {
                    ++groupLayout._dynamicBufferCountTotal;
                } else if( isImageType(universalDescriptor.type)) {
                    groupLayout._imageDescriptorType[groupLayout._imageCountTotal++] = universalDescriptor.type;
                }
                groupLayout._descriptorBindingMasks[argIndex] |= 1 << nativeDescriptor.binding;
            }
            groupLayout._dynamicBufferCount[setID] = groupLayout._dynamicBufferCountTotal - groupLayout._dynamicOffsetBaseIndex[setID];
            ///>
            VkDescriptorSetLayout setLayout = VK_NULL_HANDLE;
            VkResult rst = vkCreateDescriptorSetLayout( device->device(), &layoutInfo, nullptr, &setLayout);
            assert( rst == VK_SUCCESS );
            if( VK_SUCCESS == rst ) {
                groupLayout._descriptorSetLayouts[setID] = setLayout;
            }
        }
        std::vector<VkPushConstantRange> ranges;
        VkShaderStageFlags rangeFlagBits = 0;
        for( uint32_t i = (uint32_t)ugi::ShaderModuleType::VertexShader; i<(uint32_t)ugi::ShaderModuleType::ShaderTypeCount; ++i) {
            if( pipelineDescription.pipelineConstants[i].size ) {
                VkPushConstantRange range;
                range.size =  pipelineDescription.pipelineConstants[i].size;
                range.offset = pipelineDescription.pipelineConstants[i].offset;
                range.stageFlags = shaderStageToVk( (ShaderModuleType)i );
                rangeFlagBits |= range.stageFlags;
            }
        }
        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {}; {
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.pNext = nullptr;
            pipelineLayoutInfo.flags = 0;
            pipelineLayoutInfo.setLayoutCount = pipelineDescription.argumentCount;
            pipelineLayoutInfo.pSetLayouts = groupLayout._descriptorSetLayouts;
            pipelineLayoutInfo.pushConstantRangeCount = ranges.size();
            pipelineLayoutInfo.pPushConstantRanges = pipelineLayoutInfo.pushConstantRangeCount ? ranges.data() : nullptr;
        }
        auto rst = vkCreatePipelineLayout( device->device(), &pipelineLayoutInfo, nullptr, &groupLayout._pipelineLayout );
        assert( rst == VK_SUCCESS );
        return layoutPtr;
    }

    void ArgumentGroup::_writeDescriptorResource( const ResourceDescriptor& resource ) {
        DescriptorHandleImp h;
        h.handle = resource.descriptorHandle;
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
        case ArgumentDescriptorType::UniformBuffer: {
            VkDescriptorBufferInfo* pBufferInfo = (VkDescriptorBufferInfo*)&mixedDescriptor;
            if( pBufferInfo->buffer != resource.buffer->buffer()) {
                _reallocDescriptorSetBits |= 1 << h.setID;
                pBufferInfo->buffer = resource.buffer->buffer();
                pBufferInfo->offset = 0; // resource.bufferOffset;  这个offset为什么是0？？因为绑定的时候还会再设置一次动态offset
                pBufferInfo->range = resource.bufferRange;
                write.pBufferInfo = pBufferInfo;
            }
            pBufferInfo->range = resource.bufferRange;
            _dynamicOffsets[h.specifiedIndex] = resource.bufferOffset;
            break;
        }
        case ArgumentDescriptorType::Image:{
            VkDescriptorImageInfo* pImageInfo = (VkDescriptorImageInfo*)&mixedDescriptor;
            if( pImageInfo->imageView != resource.texture->imageView()) {
                pImageInfo->imageView = resource.texture->imageView();
                pImageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                pImageInfo->sampler = VK_NULL_HANDLE; // 再也不和sampler混合绑定了
                write.pImageInfo = pImageInfo;
            }            
            break;
        }
        case ArgumentDescriptorType::Sampler:{
            VkDescriptorImageInfo* pImageInfo = (VkDescriptorImageInfo*)&mixedDescriptor;
            pImageInfo->imageView = VK_NULL_HANDLE;
            pImageInfo->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            auto sampler = CreateSampler( _groupLayout->device()->device(), resource.sampler );
            if( pImageInfo->sampler != sampler ) {
                _reallocDescriptorSetBits |= 1 << h.setID;
                pImageInfo->sampler = sampler; // 再也不和 texture 混合绑定了
                write.pImageInfo = pImageInfo;
            }            
            break;
        }
        default:
            assert(false);// 没支持的以后再支持
            break;
        }
    }

    ArgumentGroup::ArgumentGroup( const ArgumentGroupLayout* groupLayout ) 
            : _groupLayout( groupLayout )
            , _resourceMasks {}
            , _resources {}
            , _vecMixedDesciptorInfo( groupLayout->descriptorCountTotal() )
            , _descriptorWrites( groupLayout->descriptorCountTotal() )
            , _dynamicOffsets( groupLayout->dynamicBufferCountTotal() )
            , _argumentBitMask( groupLayout->descriptorSetBitMask() )
            , _descriptorSets {}
            , _imageResources {}
            , _reallocDescriptorSetBits (0)
            , _bindPoint( VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_GRAPHICS )
        {
        }

    bool ArgumentGroup::validateIntegrility() {
        return _groupLayout->validateResourceIntegrility(_resourceMasks);
    }

    void ArgumentGroup::updateDescriptor( const ResourceDescriptor& resource ) {
        DescriptorHandleImp h;
        h.handle = resource.descriptorHandle;
        uint32_t setIndex = h.setID;
        uint32_t binding = h.binding;

        if( resource.type == ArgumentDescriptorType::Image || resource.type == ArgumentDescriptorType::StorageImage) {
            _imageResources[h.specifiedIndex] = resource.texture;
        }
        //
        _resourceMasks[setIndex] |= (1<<binding);
        _resources[setIndex][binding] = resource;
        //
        _writeDescriptorResource(resource);
    }

    bool ArgumentGroup::validateDescriptorSets() {
        if(!validateIntegrility()) {
            return false;
        }
        // 有必要就更新set
        for( uint32_t setID = 0; setID<MaxArgumentCount; ++setID) {
            if( _reallocDescriptorSetBits & 1<<setID ) {
                if( _descriptorSets[setID]) {
                    DescriptorSetAllocator::Instance()->free(_descriptorSets[setID]);
                }
                _descriptorSets[setID] = DescriptorSetAllocator::Instance()->allocate(_groupLayout->descriptorSetLayout(setID));
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
                _reallocDescriptorSetBits ^= (1<<setID);
            }
        }
        return true;
    }

    void ArgumentGroup::bind( CommandBuffer* commandBuffer ) {
        //
        VkCommandBuffer cmdbuf = *commandBuffer;
        VkPipelineLayout pipelineLayout = _groupLayout->pipelineLayout();
        VkDescriptorSet desciprotSets[MaxArgumentCount];
        uint32_t descriptorSetCount = 0;
        for( uint32_t i = 0; i< MaxArgumentCount; ++i) {
            if(_descriptorSets[i]) {
                desciprotSets[descriptorSetCount++] = _descriptorSets[i];
            }
        }
        if( descriptorSetCount ) {
            if( _dynamicOffsets.size() ) {
                vkCmdBindDescriptorSets( cmdbuf, _bindPoint, pipelineLayout, 0, descriptorSetCount, desciprotSets, _dynamicOffsets.size(), _dynamicOffsets.data() );
            } else {
                vkCmdBindDescriptorSets( cmdbuf, _bindPoint, pipelineLayout, 0, descriptorSetCount, desciprotSets, 0, nullptr );
            }
        }
    }

    const ArgumentGroupLayout* Device::getArgumentGroupLayout( const PipelineDescription& pipelineDescription, uint64_t* hashPtr ) {
        UGIHash<APHash> hasher;
        // 先 hash descriptor sets
        for( uint32_t i = 0; i < pipelineDescription.argumentCount; ++i ) {
            hasher.hashPOD(pipelineDescription.argumentLayouts[i].index);
            for( uint32_t j = 0; j<pipelineDescription.argumentLayouts[i].descriptorCount; ++j) {
                hasher.hashPOD( pipelineDescription.argumentLayouts[i].descriptors[j].binding);
                hasher.hashPOD( pipelineDescription.argumentLayouts[i].descriptors[j].dataSize);
                hasher.hashPOD( pipelineDescription.argumentLayouts[i].descriptors[j].shaderStage);
                hasher.hashPOD( pipelineDescription.argumentLayouts[i].descriptors[j].type);
            }
        }
        // hash push constants
        hasher.hashPOD(pipelineDescription.pipelineConstants);
        //
        const ArgumentGroupLayout* layout = nullptr;
        auto iter = m_argumentGroupLayoutCache.find((uint64_t)hasher);
        if( iter != m_argumentGroupLayoutCache.end()) {
            layout = iter->second;
        } else {
            layout = ArgumentGroupLayout::CreateArgumentGroupLayout(this, pipelineDescription);
            m_argumentGroupLayoutCache[(uint64_t)hasher] = layout;
        }
        if( hashPtr ) {
            *hashPtr = (uint64_t)hasher;
        }
        return layout;
    }
    //
    VkPipelineLayout GetPipelineLayout( const ArgumentGroupLayout* argGroupLayout ) {
        return argGroupLayout->pipelineLayout();
    }

    bool ArgumentGroup::prepairResource( ResourceCommandEncoder* commandEncoder ) {
        if( !validateDescriptorSets()) {
            return false;
        }
         // update all image layouts
        for( uint32_t i = 0; i < _groupLayout->imageResourceTotal(); ++i ) {
            auto type = _groupLayout->imageResourceType(i);
            if( type == ArgumentDescriptorType::Image ) {
                commandEncoder->imageTransitionBarrier( _imageResources[i], ResourceAccessType::ShaderRead, PipelineStages::Top, StageAccess::Write, PipelineStages::VertexInput, StageAccess::Read);
            } else if( type == ArgumentDescriptorType::StorageImage ) {
                commandEncoder->imageTransitionBarrier( _imageResources[i], ResourceAccessType::ShaderRead, PipelineStages::Top, StageAccess::Write, PipelineStages::VertexInput, StageAccess::Write);
            }
        }
        return true;
    }

}


