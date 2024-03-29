#include "pipeline.h"
#include <vulkan_function_declare.h>
#include <ugi_types.h>
#include <ugi_type_mapping.h>
#include <ugi_utility.h>
#include <render_pass.h>
#include <device.h>
#include <descriptor_binder.h>
#include <command_buffer.h>
#include <render_components/pipeline_material.h>
#include <material_layout.inl>

namespace ugi {

    constexpr VkDynamicState PipelineDynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR,
        VK_DYNAMIC_STATE_LINE_WIDTH,
        VK_DYNAMIC_STATE_BLEND_CONSTANTS,
        VK_DYNAMIC_STATE_STENCIL_REFERENCE,
        VK_DYNAMIC_STATE_DEPTH_BIAS
    };

    VkShaderModule CreateShaderModule( VkDevice device, const uint32_t* spirvBytes, uint32_t size, VkShaderStageFlagBits shaderStage ) {
        VkShaderModuleCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.pCode = spirvBytes;
        createInfo.codeSize = size;
        VkShaderModule module;
        auto rst = vkCreateShaderModule(device, &createInfo, nullptr, &module );
        if( VK_SUCCESS != rst ) {
            return VK_NULL_HANDLE;
        }
        return module;
    }

    GraphicsPipeline::GraphicsPipeline() 
        : _IAStateCreateInfo {}
        , _attachmnetBlendState {}
        , _renderTargetBlendStateCreateInfo {}
        , _multisampleStateCreateInfo {}
        , _viewportStateCreateInfo {}
        , _depthStencilStateCreateInfo {}
        , _tessellationStateCreateInfo {}
        , _dynamicStateCreateInfo {}
        , _vertexInputStateCreateInfo {}
    {
        
        _RSStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        _RSStateCreateInfo.pNext = NULL;
        _RSStateCreateInfo.flags = 0; 
        _RSStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL; 
        _RSStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT; 
        _RSStateCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE; 
        _RSStateCreateInfo.depthClampEnable = VK_FALSE; 
        _RSStateCreateInfo.rasterizerDiscardEnable= VK_FALSE; 
        _RSStateCreateInfo.depthBiasEnable= VK_FALSE; 
        _RSStateCreateInfo.depthBiasConstantFactor = 0;
        _RSStateCreateInfo.depthBiasClamp = 0;
        _RSStateCreateInfo.depthBiasSlopeFactor = 0; 
        _RSStateCreateInfo.lineWidth = 1.0f;
    }

    VkPipelineLayout GetPipelineLayout(const MaterialLayout* argGroupLayout);

    GraphicsPipeline* GraphicsPipeline::CreatePipeline( Device* device, const pipeline_desc_t& pipelineDescription ) {
        GraphicsPipeline* pipelinePtr = new GraphicsPipeline();
        pipelinePtr->_pipelineDesc = pipelineDescription;
        GraphicsPipeline& pipeline = *pipelinePtr;
        // 1. shader stages
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
		for (uint32_t i = 0; i < (uint32_t)shader_stage_t::ShaderStageCount; ++i) {
			if ( pipelineDescription.shaders[i].spirvData) {
                VkShaderStageFlagBits stage = shaderStageToVk(pipelineDescription.shaders[i].type);
				VkPipelineShaderStageCreateInfo info = {};
				info.flags = 0;
				info.pName = "main";
				info.module = CreateShaderModule( device->device(), (uint32_t*)pipelineDescription.shaders[i].spirvData, pipelineDescription.shaders[i].spirvSize, stage );
				info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
				info.stage = stage;
				shaderStages.push_back(info);
			}
        }
        pipeline._shaderStages = shaderStages;
        pipeline._pipelineCreateInfo.stageCount = shaderStages.size();
        pipeline._pipelineCreateInfo.pStages = shaderStages.data();
        // 2. IAState
		pipeline._IAStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        pipeline._IAStateCreateInfo.pNext = nullptr;
        pipeline._IAStateCreateInfo.flags = 0;
        pipeline._IAStateCreateInfo.primitiveRestartEnable = false;

		if ( pipelineDescription.shaders[(uint32_t)shader_stage_t::TessellationControlShader].spirvData ) {
			pipeline._IAStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_PATCH_LIST;
		}
		else {
			pipeline._IAStateCreateInfo.topology = topolotyToVk( pipelineDescription.topologyMode);
		}

        VkPipelineColorBlendAttachmentState& colorAttachmentBlendState = pipeline._attachmnetBlendState[0]; {
            colorAttachmentBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorAttachmentBlendState.blendEnable = pipelineDescription.renderState.blendState.enable;
            colorAttachmentBlendState.alphaBlendOp = blendOpToVk(pipelineDescription.renderState.blendState.op);
            colorAttachmentBlendState.colorBlendOp = blendOpToVk(pipelineDescription.renderState.blendState.op);
            colorAttachmentBlendState.dstAlphaBlendFactor = blendFactorToVk(pipelineDescription.renderState.blendState.dstFactor);
            colorAttachmentBlendState.dstColorBlendFactor = blendFactorToVk(pipelineDescription.renderState.blendState.dstFactor);
            colorAttachmentBlendState.srcAlphaBlendFactor = blendFactorToVk(pipelineDescription.renderState.blendState.srcFactor);
            colorAttachmentBlendState.srcColorBlendFactor = blendFactorToVk(pipelineDescription.renderState.blendState.srcFactor);
            colorAttachmentBlendState.colorWriteMask = pipelineDescription.renderState.writeMask;
            //
            pipeline._attachmnetBlendState[1] = pipeline._attachmnetBlendState[0];
            pipeline._attachmnetBlendState[2] = pipeline._attachmnetBlendState[1];
            pipeline._attachmnetBlendState[3] = pipeline._attachmnetBlendState[2];
        }
		VkPipelineColorBlendStateCreateInfo& colorBlendState = pipeline._renderTargetBlendStateCreateInfo;
		colorBlendState.pNext = nullptr;
		colorBlendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlendState.attachmentCount = 0;
		colorBlendState.pAttachments = &pipeline._attachmnetBlendState[0];
        // ============================================================================
		VkPipelineViewportStateCreateInfo& viewportState = pipeline._viewportStateCreateInfo;
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;
        // ============================================================================
		VkPipelineDynamicStateCreateInfo& dynamicState = pipeline._dynamicStateCreateInfo; {
            dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
            dynamicState.pDynamicStates = PipelineDynamicStates;
            dynamicState.dynamicStateCount = sizeof(PipelineDynamicStates)/sizeof(PipelineDynamicStates[0]);
        }

		VkPipelineDepthStencilStateCreateInfo& depthStencilState = pipeline._depthStencilStateCreateInfo; {
			depthStencilState.pNext = nullptr;
			depthStencilState.flags = 0;
			depthStencilState.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthStencilState.depthTestEnable = pipelineDescription.renderState.depthState.testable;
			depthStencilState.depthWriteEnable = pipelineDescription.renderState.depthState.writable;
			depthStencilState.depthCompareOp = compareOpToVk(pipelineDescription.renderState.depthState.cmpFunc);
			depthStencilState.depthBoundsTestEnable = VK_FALSE;
			depthStencilState.back.failOp = stencilOpToVk(pipelineDescription.renderState.stencilState.opFail);
			depthStencilState.back.depthFailOp = stencilOpToVk(pipelineDescription.renderState.stencilState.opZFail);
			depthStencilState.back.passOp = stencilOpToVk(pipelineDescription.renderState.stencilState.opPass);
			depthStencilState.back.writeMask = pipelineDescription.renderState.stencilState.mask;
			depthStencilState.back.compareMask = 0xff;
			depthStencilState.back.compareOp = compareOpToVk(pipelineDescription.renderState.stencilState.cmpFunc);
			depthStencilState.minDepthBounds = 0;
			depthStencilState.maxDepthBounds = 1.0;
			depthStencilState.stencilTestEnable = pipelineDescription.renderState.stencilState.enable;
			depthStencilState.front = depthStencilState.back;
			depthStencilState.back.failOp = stencilOpToVk(pipelineDescription.renderState.stencilState.opFailCCW);
			depthStencilState.back.depthFailOp = stencilOpToVk(pipelineDescription.renderState.stencilState.opZFailCCW);
			depthStencilState.back.passOp = stencilOpToVk(pipelineDescription.renderState.stencilState.opPassCCW);
		}
		VkPipelineMultisampleStateCreateInfo& multisampleState = pipeline._multisampleStateCreateInfo; {
            multisampleState.pNext = nullptr;
            multisampleState.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampleState.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            multisampleState.pSampleMask = nullptr;
        }
        //
		for (uint32_t i = 0; i < pipelineDescription.vertexLayout.bufferCount; ++i)
		{
			pipeline._vertexInputBindings[i].binding = i;
			pipeline._vertexInputBindings[i].stride = pipelineDescription.vertexLayout.buffers[i].stride;
			pipeline._vertexInputBindings[i].inputRate = pipelineDescription.vertexLayout.buffers[i].instanceMode ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;
		}

		for (uint32_t i = 0; i < pipelineDescription.vertexLayout.bufferCount; ++i)
		{
			pipeline._vertexInputAttributs[i].binding = i;
			pipeline._vertexInputAttributs[i].location = i;
			pipeline._vertexInputAttributs[i].format = vertexFormatToVk(pipelineDescription.vertexLayout.buffers[i].type);
			pipeline._vertexInputAttributs[i].offset = 0;
		}
		// Vertex input state used for pipeline creation
		VkPipelineVertexInputStateCreateInfo& vertexInputState = pipeline._vertexInputStateCreateInfo;
		vertexInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputState.pNext = nullptr;
        vertexInputState.flags = 0;
		vertexInputState.vertexBindingDescriptionCount = pipelineDescription.vertexLayout.bufferCount;
		vertexInputState.pVertexBindingDescriptions = &pipeline._vertexInputBindings[0];
		vertexInputState.vertexAttributeDescriptionCount = pipelineDescription.vertexLayout.bufferCount;
		vertexInputState.pVertexAttributeDescriptions = pipeline._vertexInputAttributs.data();

        VkPipelineTessellationStateCreateInfo& tessellationState = pipeline._tessellationStateCreateInfo; {
            tessellationState.flags = 0;
            tessellationState.pNext = nullptr;
            tessellationState.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
            tessellationState.patchControlPoints = pipelineDescription.tessPatchCount;
        }
        //
        pipeline._pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipeline._pipelineCreateInfo.pNext = nullptr;
        pipeline._pipelineCreateInfo.flags = 0;
        pipeline._pipelineCreateInfo.pStages = pipeline._shaderStages.data();
		pipeline._pipelineCreateInfo.stageCount =pipeline._shaderStages.size();
		pipeline._pipelineCreateInfo.pVertexInputState = &pipeline._vertexInputStateCreateInfo;
		pipeline._pipelineCreateInfo.pInputAssemblyState = &pipeline._IAStateCreateInfo;
		pipeline._pipelineCreateInfo.pColorBlendState = &colorBlendState;
		pipeline._pipelineCreateInfo.pMultisampleState = &multisampleState;
		pipeline._pipelineCreateInfo.pViewportState = &pipeline._viewportStateCreateInfo;
		pipeline._pipelineCreateInfo.pDepthStencilState = &depthStencilState;
		pipeline._pipelineCreateInfo.pTessellationState = pipelineDescription.tessPatchCount ? &tessellationState : nullptr;
		pipeline._pipelineCreateInfo.pDynamicState = &dynamicState;
        const MaterialLayout* argumentGroupLayout = device->getPipelineMaterialLayout(pipelineDescription, pipeline._pipelineLayoutHash);
        pipeline._pipelineCreateInfo.layout = ugi::GetPipelineLayout(argumentGroupLayout);
        // 动态的，先置空
        pipeline._pipelineCreateInfo.pRasterizationState = &pipeline._RSStateCreateInfo;
		pipeline._pipelineCreateInfo.renderPass = VK_NULL_HANDLE;
        pipeline._pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipeline._pipelineCreateInfo.basePipelineIndex = -1;
        pipeline._device = device;
        // pipeline create info 准备好了
        auto argGroup = pipelinePtr->createArgumentGroup();
        pipelinePtr->_descriptorBinder = argGroup;
        return pipelinePtr;
    }

    VkPipeline GraphicsPipeline::preparePipelineStateObject( UGIHash<APHash>& hasher, const RenderCommandEncoder* encoder ) 
    {
        const IRenderPass* renderPass = encoder->renderPass();
        uint32_t currentSubpass = encoder->subpass();
        uint64_t subpassHash = renderPass->subpassHash(currentSubpass);
        //
        hasher.hashPOD(subpassHash);
        uint64_t hashValue = hasher;
        // 
        auto iter = this->_pipelineTable.find( hashValue );
        if( iter != _pipelineTable.end()) {
            return iter->second;
        }
        VkPipeline pipeline;
        // 配置 pipeline create info
        _renderTargetBlendStateCreateInfo.attachmentCount = renderPass->subpassColorAttachmentCount(currentSubpass);
        _pipelineCreateInfo.renderPass = renderPass->renderPass();
        _pipelineCreateInfo.subpass = currentSubpass;
        // 创建 pipeline
        auto rst = vkCreateGraphicsPipelines( encoder->commandBuffer()->device(), VK_NULL_HANDLE, 1, &_pipelineCreateInfo, nullptr, &pipeline );
        if( rst != VK_SUCCESS ) {
            return VK_NULL_HANDLE;
        }
        _pipelineTable[hashValue] = pipeline;
        return pipeline;
    }

    void GraphicsPipeline::setRasterizationState( const raster_state_t& _state ) {
        _RSStateCreateInfo.polygonMode = polygonModeToVk(_state.polygonMode); 
        _RSStateCreateInfo.cullMode = cullModeToVk(_state.cullMode); 
        _RSStateCreateInfo.frontFace = frontFaceToVk(_state.frontFace); 
        _RSStateCreateInfo.depthClampEnable = _state.depthBiasEnabled; 
        _RSStateCreateInfo.depthBiasEnable= _state.depthBiasEnabled; 
        _RSStateCreateInfo.depthBiasConstantFactor = _state.depthBiasConstantFactor;
        _RSStateCreateInfo.depthBiasClamp = _state.depthBiasClamp;
        _RSStateCreateInfo.depthBiasSlopeFactor = _state.depthBiasSlopeFactor;
    }

    void GraphicsPipeline::_hashRasterizationState( UGIHash<APHash>& hasher ) {
        hasher.hashPOD(_RSStateCreateInfo.polygonMode);
        hasher.hashPOD(_RSStateCreateInfo.cullMode);
        hasher.hashPOD(_RSStateCreateInfo.frontFace);
        hasher.hashPOD(_RSStateCreateInfo.depthClampEnable);
        hasher.hashPOD(_RSStateCreateInfo.depthBiasEnable);
        // 
        if(_RSStateCreateInfo.depthBiasEnable) {
            hasher.hashPOD(_RSStateCreateInfo.depthBiasConstantFactor);
            hasher.hashPOD(_RSStateCreateInfo.depthBiasClamp);
            hasher.hashPOD(_RSStateCreateInfo.depthBiasSlopeFactor);
        }
    }

    void GraphicsPipeline::bind(RenderCommandEncoder* encoder) {
        UGIHash<APHash> hasher;
        _hashRasterizationState(hasher);
        VkPipeline pipeline = preparePipelineStateObject( hasher,  encoder );
        vkCmdBindPipeline(*encoder->commandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
    }

    DescriptorBinder* GraphicsPipeline::createArgumentGroup() const {
        const auto argumentLayout = this->_device->getPipelineMaterialLayout(_pipelineLayoutHash);
        assert(argumentLayout);
        if( !argumentLayout) {
            return nullptr;
        }
        DescriptorBinder* group = new DescriptorBinder(argumentLayout, _device->descriptorSetAllocator());
        return group;        
    }

    uint32_t GraphicsPipeline::getDescriptorHandle(char const* descriptorName, res_descriptor_info_t* descriptorInfo) const {
        DescriptorHandleImp handle;
        handle.descriptorIndex = 0;
        handle.specifiedIndex = 0;
        handle.binding = 0;
        handle.handle = 0;
        //
        uint32_t dynamicBufferIndex = 0;
        uint32_t imageIndex = 0;

        for( uint32_t argIndex = 0; argIndex< _pipelineDesc.argumentCount; ++argIndex) {
            auto setIndex = _pipelineDesc.argumentLayouts[argIndex].index;
            handle.setID = setIndex;
            handle.binding = 0;
            for( uint32_t descriptorIndex = 0; descriptorIndex < _pipelineDesc.argumentLayouts[argIndex].descriptorCount; ++descriptorIndex) {
                const auto& descriptor = _pipelineDesc.argumentLayouts[argIndex].descriptors[descriptorIndex];
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
                    if(descriptorInfo) {
                        *descriptorInfo = descriptor;
                    }
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

    Material* GraphicsPipeline::createMaterial(std::vector<std::string> const& parameters, std::vector<res_union_t> const& resources) {
        if(resources.size()) {
            if(resources.size()!=parameters.size()) {
                return nullptr;
            }
        }
        Material* mtl = new Material();
        for(size_t i = 0; i<parameters.size(); ++i) {
            auto const& name = parameters[i];
            res_descriptor_info_t descInfo;
            auto handle = this->getDescriptorHandle(name.c_str(), &descInfo);
            if(handle != ~0) {
                res_descriptor_t descriptor;
                descriptor.handle = handle;
                descriptor.type = descInfo.type;
                if(descriptor.type == res_descriptor_type::UniformBuffer) {
                    descriptor.res.buffer.size = descInfo.dataSize;
                    descriptor.res.buffer.size = descInfo.dataSize;
                }
                if(resources.size()) {
                    if(descriptor.type == res_descriptor_type::UniformBuffer) {
                        descriptor.res.buffer.buffer = resources[i].buffer.buffer;
                        descriptor.res.buffer.offset = resources[i].buffer.offset;
                    } else {
                        descriptor.res = resources[i];
                    }
                }
                mtl->descriptors_.push_back(descriptor);
            }
        }
        return mtl;
    }

    void GraphicsPipeline::applyMaterial(Material const* material) {
        for(auto const& item: material->descriptors()) {
            _descriptorBinder->updateDescriptor(item);
        }
    }

    void GraphicsPipeline::flushMaterials(CommandBuffer const* cmd) {
        this->_descriptorBinder->bind(cmd);
    }

    void GraphicsPipeline::resetMaterials() {
        _descriptorBinder->reset();
    }

    ComputePipeline* ComputePipeline::CreatePipeline( Device* device, const pipeline_desc_t& pipelineDesc ) {
        uint64_t pipelineLayoutHash;
        VkComputePipelineCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.basePipelineHandle = VK_NULL_HANDLE;
        createInfo.basePipelineIndex = 0;
        auto pipelineLayout = device->getPipelineMaterialLayout(pipelineDesc, pipelineLayoutHash);
        createInfo.layout = ugi::GetPipelineLayout(pipelineLayout);
        VkPipelineShaderStageCreateInfo stageInfo; {
            stageInfo.flags = 0;
            stageInfo.pName = "main";
            uint32_t* spirvData = (uint32_t*)pipelineDesc.shaders[(uint32_t)shader_stage_t::ComputeShader].spirvData;
            uint32_t sprivDataLength = pipelineDesc.shaders[(uint32_t)shader_stage_t::ComputeShader].spirvSize;
            stageInfo.module = CreateShaderModule( device->device(), spirvData, sprivDataLength, (VkShaderStageFlagBits)VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT );
            stageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            stageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
            stageInfo.pSpecializationInfo = nullptr;
            stageInfo.pNext = nullptr;
        }
        createInfo.stage = stageInfo;
        VkPipeline pipeline;
        VkResult rst = vkCreateComputePipelines( device->device(), VK_NULL_HANDLE, 1, &createInfo, nullptr, &pipeline);
        if( rst == VK_SUCCESS ) {
            ComputePipeline* computePipeline = new ComputePipeline();
            computePipeline->_device = device;
            computePipeline->_pipeline = pipeline;
            computePipeline->_pipelineLayoutHash = pipelineLayoutHash;
            computePipeline->_createInfo = createInfo;
            computePipeline->_descriptorBinder = computePipeline->createArgumentGroup();
            computePipeline->_pipelineDesc = pipelineDesc;
            return computePipeline;
        }
        return nullptr;
    }

    DescriptorBinder* ComputePipeline::createArgumentGroup() const  {
        const auto argumentLayout = _device->getPipelineMaterialLayout(_pipelineLayoutHash);
        assert(argumentLayout);
        if( !argumentLayout) {
            return nullptr;
        }
        DescriptorBinder* group = new DescriptorBinder(argumentLayout, _device->descriptorSetAllocator(), VkPipelineBindPoint::VK_PIPELINE_BIND_POINT_COMPUTE);
        return group;
    }

    Material* ComputePipeline::createMaterial(std::vector<std::string> const& parameters, std::vector<res_union_t> const& resources) {
        if(resources.size()) {
            if(resources.size()!=parameters.size()) {
                return nullptr;
            }
        }
        Material* mtl = new Material();
        for(size_t i = 0; i<parameters.size(); ++i) {
            auto const& name = parameters[i];
            res_descriptor_info_t descInfo;
            auto handle = this->getDescriptorHandle(name.c_str(), &descInfo);
            if(handle != ~0) {
                res_descriptor_t descriptor;
                descriptor.handle = handle;
                descriptor.type = descInfo.type;
                if(descriptor.type == res_descriptor_type::UniformBuffer) {
                    descriptor.res.buffer.size = descInfo.dataSize;
                    descriptor.res.buffer.size = descInfo.dataSize;
                }
                if(resources.size()) {
                    if(descriptor.type == res_descriptor_type::UniformBuffer) {
                        descriptor.res.buffer.buffer = resources[i].buffer.buffer;
                        descriptor.res.buffer.offset = resources[i].buffer.offset;
                    } else {
                        descriptor.res = resources[i];
                    }
                }
                mtl->descriptors_.push_back(descriptor);
            }
        }
        return mtl;
    }

    void ComputePipeline::applyMaterial(Material const* material) {
        for(auto const& item: material->descriptors()) {
            _descriptorBinder->updateDescriptor(item);
        }
    }

    void ComputePipeline::flushMaterials(CommandBuffer const* cmd) {
        this->_descriptorBinder->bind(cmd);
    }

    void ComputePipeline::resetMaterials() {
        this->_descriptorBinder->reset();
    }

    VkPipeline ComputePipeline::pipeline() {
        return _pipeline;
    }

    uint32_t ComputePipeline::getDescriptorHandle(char const* descriptorName, res_descriptor_info_t* descriptorInfo) const {
        DescriptorHandleImp handle;
        handle.descriptorIndex = 0;
        handle.specifiedIndex = 0;
        handle.binding = 0;
        handle.handle = 0;
        //
        uint32_t dynamicBufferIndex = 0;
        uint32_t imageIndex = 0;

        for( uint32_t argIndex = 0; argIndex< _pipelineDesc.argumentCount; ++argIndex) {
            auto setIndex = _pipelineDesc.argumentLayouts[argIndex].index;
            handle.setID = setIndex;
            for( uint32_t descriptorIndex = 0; descriptorIndex < _pipelineDesc.argumentLayouts[argIndex].descriptorCount; ++descriptorIndex) {
                const auto& descriptor = _pipelineDesc.argumentLayouts[argIndex].descriptors[descriptorIndex];
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
                    if(descriptorInfo) {
                        *descriptorInfo = descriptor;
                    }
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

}