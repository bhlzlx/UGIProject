#pragma once

#include "VulkanDeclare.h"
#include "UGIDeclare.h"
#include "UGITypes.h"
#include <unordered_map>
#include <vector>
#include <array>
#include <cstdint>
#include "UGIUtility.h"

namespace ugi {

    class GraphicsPipeline {
    private:
        Device*                                             _device;
        pipeline_desc_t                                     _pipelineDesc;
        std::unordered_map<uint64_t, VkPipeline>            _pipelineTable;
        //
        std::vector<VkPipelineShaderStageCreateInfo>        _shaderStages;
        VkPipelineInputAssemblyStateCreateInfo              _IAStateCreateInfo;
        VkPipelineColorBlendAttachmentState                 _attachmnetBlendState[MaxRenderTarget];
        VkPipelineColorBlendStateCreateInfo                 _renderTargetBlendStateCreateInfo;
        VkPipelineMultisampleStateCreateInfo                _multisampleStateCreateInfo;
        VkPipelineViewportStateCreateInfo                   _viewportStateCreateInfo;
        VkPipelineDepthStencilStateCreateInfo               _depthStencilStateCreateInfo;
        VkPipelineTessellationStateCreateInfo               _tessellationStateCreateInfo;
        VkPipelineDynamicStateCreateInfo                    _dynamicStateCreateInfo;
        VkPipelineVertexInputStateCreateInfo                _vertexInputStateCreateInfo;
        //
        std::array<VkVertexInputBindingDescription, 16>     _vertexInputBindings;
		std::array<VkVertexInputAttributeDescription, 16>   _vertexInputAttributs;
        // 因为在逻辑里我们会经常改变pipeline的这个属性，所以我们将处理成动态，通过hash得到不同的pipeline object
        VkPipelineRasterizationStateCreateInfo              _RSStateCreateInfo;                     ///> 这个不会用到的，只是放着给参考一下
        VkGraphicsPipelineCreateInfo                        _pipelineCreateInfo;
        uint64_t                                            _pipelineLayoutHash;                    ///> pipeline layout 的 hash
        MaterialLayout*                                     _materialLayout;
        DescriptorBinder*                                   _descriptorBinder;
    private:
        VkPipeline preparePipelineStateObject(UGIHash<APHash>& hasher, const RenderCommandEncoder* encoder);
        void _hashRasterizationState(UGIHash<APHash>& hasher);
        DescriptorBinder* createArgumentGroup() const;
    public:
        static GraphicsPipeline* CreatePipeline(Device* device, const pipeline_desc_t& pipelineDescription);

        GraphicsPipeline();

        void setRasterizationState(const raster_state_t& _state);
        void setDepthStencilState();
        void bind(RenderCommandEncoder* encoder);
        void bind(ComputeCommandEncoder* encoder);
        void applyMaterial(Material const* material);
        void flushMaterials(CommandBuffer const* cmd);
        void resetMaterials();
        Material* createMaterial(std::vector<std::string> const& parameters, std::vector<res_union_t> const& resources);
        uint32_t getDescriptorHandle(char const* descriptorName, res_descriptor_info_t* descriptorInfo = nullptr) const;
        pipeline_desc_t const& desc() const {
            return _pipelineDesc;
        }
        //
        // DescriptorBinder* argumentBinder() const {
        //     return _descriptorBinder;
        // }
    };


    class ComputePipeline {
    private:
        Device*                                             _device;
        VkPipeline                                          _pipeline;
        VkComputePipelineCreateInfo                         _createInfo;
        uint64_t                                            _pipelineLayoutHash;
    public:
        ComputePipeline() {
        }
        static ComputePipeline* CreatePipeline( Device* device, const pipeline_desc_t& pipelineDesc );
        DescriptorBinder* createArgumentGroup() const ;
        VkPipeline pipeline() {
            return _pipeline;
        }     
    };

}