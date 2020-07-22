#include "UGIUtility.h"
#include "Descriptor.h"
#include "ArgumentGroupLayout.inl"
#include "Device.h"
#include "UGITypeMapping.h"
#include "hashObjectPool/HashObjectPool.h"

namespace ugi {

    class ArgumentGroupHashMethod {
    private:
    public:
        uint64_t operator()( const PipelineDescription& pipDesc ) {
            UGIHash<APHash> hasher;
            // 先 hash descriptor sets
            for( uint32_t i = 0; i < pipDesc.argumentCount; ++i ) {
                hasher.hashPOD(pipDesc.argumentLayouts[i].index);
                for( uint32_t j = 0; j<pipDesc.argumentLayouts[i].descriptorCount; ++j) {
                    hasher.hashPOD( pipDesc.argumentLayouts[i].descriptors[j].binding);
                    hasher.hashPOD( pipDesc.argumentLayouts[i].descriptors[j].dataSize);
                    hasher.hashPOD( pipDesc.argumentLayouts[i].descriptors[j].shaderStage);
                    hasher.hashPOD( pipDesc.argumentLayouts[i].descriptors[j].type);
                }
            }
            // hash push constants
            hasher.hashPOD(pipDesc.pipelineConstants);
            return hasher;
        }
    };

    class ArgumentGroupCreateMethod {
    private:
    public:
        ArgumentGroupLayout* operator()( Device* device, const PipelineDescription& pipelineDescription ) {
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
    };

    using ArgumentGroupLayoutPool = HashObjectPool< PipelineDescription, ArgumentGroupLayout*, Device*, ArgumentGroupHashMethod, ArgumentGroupCreateMethod>;

    const ArgumentGroupLayout* Device::getArgumentGroupLayout( const PipelineDescription& pipelineDescription, uint64_t& hashVal ) {
        auto argumentGroupLayout = ArgumentGroupLayoutPool::GetInstance()->getObject(pipelineDescription, this, hashVal);
        return argumentGroupLayout;
    }

    const ArgumentGroupLayout* Device::getArgumentGroupLayout( uint64_t hashVal ) {
        auto argumentGroupLayout = ArgumentGroupLayoutPool::GetInstance()->getObject(hashVal);
        return argumentGroupLayout;
    }

}