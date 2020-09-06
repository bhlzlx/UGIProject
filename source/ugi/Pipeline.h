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

    class Pipeline {
    private:
        Device*                                             _device;
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
        std::array< VkVertexInputBindingDescription, 16 >   _vertexInputBindings;
		std::array<VkVertexInputAttributeDescription, 16 >  _vertexInputAttributs;
        // 因为在逻辑里我们会经常改变pipeline的这个属性，所以我们将处理成动态，通过hash得到不同的pipeline object
        VkPipelineRasterizationStateCreateInfo              _RSStateCreateInfo;                     ///> 这个不会用到的，只是放着给参考一下
        VkGraphicsPipelineCreateInfo                        _pipelineCreateInfo;
        uint64_t                                            _pipelineLayoutHash;                    ///> pipeline layout 的 hash
    private:

        VkPipeline preparePipelineStateObject( UGIHash<APHash>& hasher, const RenderCommandEncoder* encoder );
        void _hashRasterizationState( UGIHash<APHash>& hasher );
    public:
        static Pipeline* CreatePipeline( Device* device, const PipelineDescription& pipelineDescription);

        Pipeline();

        void setRasterizationState( const RasterizationState& _state );
        void setDepthStencilState(  );
        void bind( RenderCommandEncoder* encoder );
        void bind( ComputeCommandEncoder* encoder );
        //
        ArgumentGroup* createArgumentGroup() const;
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
        static ComputePipeline* CreatePipeline( Device* device, const PipelineDescription& pipelineDesc );
        ArgumentGroup* createArgumentGroup() const ;
        VkPipeline pipeline() {
            return _pipeline;
        }
    };

}