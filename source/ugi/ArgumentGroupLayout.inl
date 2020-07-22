#include "UGITypes.h"

namespace ugi {

    static inline bool isDynamicBufferType( ArgumentDescriptorType type ) {
        if( type == ArgumentDescriptorType::UniformBuffer ) {
            return true;
        }
        return false;
    }

    static inline bool isImageType( ArgumentDescriptorType type ) {
        if( type == ArgumentDescriptorType::Image || type == ArgumentDescriptorType::StorageImage ) {
            return true;
        }
        return false;
    }
    /* ===============================================================================================================
    *   Argument Group Layout
    *   只存参数的Layout信息
    * ==============================================================================================================*/
    class ArgumentGroupLayout
    {
    public:
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
        // static ArgumentGroupLayout* CreateArgumentGroupLayout( Device* device, const PipelineDescription& pipelineDescription );
    };
}