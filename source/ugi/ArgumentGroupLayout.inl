#include "UGITypes.h"

namespace ugi {

    static inline bool isDynamicBufferType( res_descriptor_type type ) {
        if( type == res_descriptor_type::UniformBuffer ) {
            return true;
        }
        return false;
    }

    static inline bool isImageType( res_descriptor_type type ) {
        if( type == res_descriptor_type::Image || type == res_descriptor_type::StorageImage ) {
            return true;
        }
        return false;
    }
    /* ===============================================================================================================
    *   Argument Group Layout
    *   只存参数的Layout信息
    * ==============================================================================================================*/
    class MaterialLayout
    {
    public:
        Device*                 _device;
        // descriptor set
        uint32_t                _descriptorSetCount;                        ///> descriptor set 数量
        uint32_t                _descriptorSetBitMask;                      ///> 用来确定有哪些 descriptor set layout
        VkDescriptorSetLayout   _descriptorSetLayouts[MaxArgumentCount];
        // descriptors
        uint32_t                _descriptorCountTotal;                      ///> 所有的 descriptor 数量
        // 32bit mask per descriptor set
        uint32_t                _descriptorBindingMasks[MaxArgumentCount]; 
        // descriptor count per descriptor set
        uint32_t                _descriptorCount[MaxArgumentCount];       ///> 每个 set 有几个 descriptor ?
        uint32_t                _descriptorSetWriteBaseIndex[MaxArgumentCount]; ///> argument里有很多 VkWriteDescriptorSet对象，每个set占用若干个，这里记录set对应的起始index
        // 特殊用途
        uint32_t                _dynamicBufferCountTotal;                   ///> 比如说 uniform buffer 这种动态偏移绑定的
        uint32_t                _dynamicBufferCount[MaxArgumentCount];      ///> 每个set有多少个 dynamic descriptor
        uint32_t                _dynamicOffsetBaseIndex[MaxArgumentCount];  ///> 把所有的dynamic offset存数组里，每个set的 起始 dynamic descriptor 对应的索引
        //
        uint32_t                _imageCountTotal;
        res_descriptor_type     _imageDescriptorType[MaxDescriptorCount];   ///> 这个量是随便写的，不过一个管线一般也不会超过8张纹理吧！
        //
        VkPipelineLayout        _pipelineLayout;
    public:
        MaterialLayout();
        // ===============================================================================================================
        MaterialLayout( MaterialLayout&& materialLayout);
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
        res_descriptor_type imageResourceType( uint32_t idx ) const {
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
        // static MaterialLayout* CreateArgumentGroupLayout( Device* device, const PipelineDescription& pipelineDescription );
    };

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
}