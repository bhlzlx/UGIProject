#include "VulkanFunctionDeclare.h"
#include "CommandBuffer.h"
#include "UGIDeclare.h"
#include "RenderPass.h"
#include "Buffer.h"
#include "Texture.h"
#include "Pipeline.h"
#include "Drawable.h"
#include "UGITypeMapping.h"
#include "Argument.h"
#include <vector>

namespace ugi {

    VkAccessFlags GetBarrierAccessMask( PipelineStages stage, StageAccess stageTransition ) {
        struct BarrierAccessMask {
                VkAccessFlags accessFlags[2];
            BarrierAccessMask( uint32_t src1, uint32_t dst1) 
                : accessFlags { src1, dst1 }
            {
            }
        };
        BarrierAccessMask accessMaskTable[] = {
            { 0, 0 },
            { VK_ACCESS_INDIRECT_COMMAND_READ_BIT, 0},
            { VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT, 0},
            { VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT }, // vertex
            { VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT }, // tess control
            { VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT }, // tess eval
            { VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT }, // geom
            { VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT }, // fragment
            { VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT }, // earyFramgnet
            { VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT }, // lateFragment
            { VK_ACCESS_COLOR_ATTACHMENT_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT },
            { VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT }, // compute
            { VK_ACCESS_TRANSFER_READ_BIT ,VK_ACCESS_TRANSFER_WRITE_BIT },
            { 0, 0 },
        };
        switch( stage ) {
            case PipelineStages::Top:
                return accessMaskTable[0].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::DrawIndirect:
                return accessMaskTable[1].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::VertexInput:
                return accessMaskTable[2].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::VertexShading:
                return accessMaskTable[3].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::TessControlShading:
                return accessMaskTable[4].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::TessEvaluationShading:
                return accessMaskTable[5].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::GeometryShading:
                return accessMaskTable[6].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::FragmentShading:
                return accessMaskTable[7].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::EaryFragmentTestShading:
                return accessMaskTable[8].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::LateFragmmentTestShading:
                return accessMaskTable[9].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::ColorAttachmentOutput:
                return accessMaskTable[10].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::ComputeShading:
                return accessMaskTable[11].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::Transfer:
                return accessMaskTable[12].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::Bottom:
                return accessMaskTable[13].accessFlags[(uint32_t)stageTransition];
            case PipelineStages::Host:
                return accessMaskTable[13].accessFlags[(uint32_t)stageTransition];
            default: 
                assert( false );
                return 0;
        }
        return 0;
    }

    CommandBuffer::CommandBuffer( VkDevice device, VkCommandBuffer cb ) 
        : _cmdbuff( cb )
        , _device(device)
        , _encodeState(0)
    {
    }

    CommandBuffer::operator VkCommandBuffer() const {
        return _cmdbuff;
    }

    VkDevice CommandBuffer::device() const {
        return _device;
    }

    ResourceCommandEncoder* CommandBuffer::resourceCommandEncoder() {
        if( _encodeState ) {
            return nullptr;
        }
        new(&_resourceEncoder) ResourceCommandEncoder(this);
        return &_resourceEncoder;
    }

    RenderCommandEncoder* CommandBuffer::renderCommandEncoder( IRenderPass* renderPass ) {
        if( _encodeState ) {
            return nullptr;
        }
        new(&_renderEncoder) RenderCommandEncoder(this, renderPass);
        renderPass->begin(&_renderEncoder);
        return &_renderEncoder;
    }

    void CommandBuffer::reset() {
        vkResetCommandBuffer(_cmdbuff, 0);
    }

    void CommandBuffer::beginEncode() {
        VkCommandBufferBeginInfo beginInfo = {}; {
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.pNext = nullptr;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            beginInfo.pInheritanceInfo = nullptr;
        }
        vkBeginCommandBuffer(_cmdbuff, &beginInfo);
    }

    void CommandBuffer::endEncode() {
        vkEndCommandBuffer(_cmdbuff);
    }

}