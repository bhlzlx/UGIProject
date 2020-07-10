//
#include "VulkanFunctionDeclare.h"
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
//
#include "RenderPass.h"
#include "CommandBuffer.h"

#include "Device.h"
#include "Texture.h"
#include "UGITypeMapping.h"
#include <cassert>
#include "UGIUtility.h"

namespace ugi {

    uint64_t RenderPassObjectManager::CompatibleHashFunc( const RenderPassDescription& _subpassDesc, uint32_t _subpassIndex ) {
        UGIHash<APHash> hasher;
        if (_subpassDesc.colorAttachmentCount) {
            for (uint32_t i = 0; i < _subpassDesc.colorAttachmentCount; ++i) {
                hasher.hashPOD(_subpassDesc.colorAttachments[i].format);
                hasher.hashPOD(_subpassDesc.colorAttachments[i].multisample);
            }
        }
        if (_subpassDesc.depthStencil.format != UGIFormat::InvalidFormat) {
            hasher.hashPOD(_subpassDesc.depthStencil.format);
            hasher.hashPOD(_subpassDesc.depthStencil.multisample);
        }
        if (_subpassDesc.inputAttachmentCount) {
            for (uint32_t i = 0; i < _subpassDesc.inputAttachmentCount; ++i) {
                hasher.hashPOD(_subpassDesc.inputAttachments[i].format);
                hasher.hashPOD(_subpassDesc.inputAttachments[i].multisample);
            }
        }
        if (_subpassDesc.resolve.format != UGIFormat::InvalidFormat) {
            hasher.hashPOD(_subpassDesc.resolve.format);
            hasher.hashPOD(_subpassDesc.resolve.multisample);
        }
        hasher.hashPOD(_subpassIndex);
        return hasher;
    }

    VkRenderPass RenderPassObjectManager::getRenderPass( Device* _device,  const RenderPassDescription& _desc, uint64_t& hash  ) {
        auto sampleCount = VK_SAMPLE_COUNT_1_BIT; // 暂时先写死，以后再改
        std::vector<VkAttachmentDescription> attacmentDescriptions; {
            for (uint32_t i = 0; i < _desc.colorAttachmentCount; ++i) {
                VkAttachmentDescription desc; {
                    desc.flags = 0;
                    desc.format = UGIFormatToVk(_desc.colorAttachments[i].format);
                    desc.samples = sampleCount;
                    desc.loadOp = LoadOpToVk(_desc.colorAttachments[i].loadAction);
                    desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // does not support tiled rendering
                    desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // color attachment dose not contains this
                    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
                    desc.initialLayout = imageLayoutFromAccessType(_desc.colorAttachments[i].initialAccessType);
                    desc.finalLayout = imageLayoutFromAccessType(_desc.colorAttachments[i].finalAccessType);
                }
                attacmentDescriptions.push_back(desc);
            }
            {
                VkAttachmentDescription desc; {
                    desc.flags = 0;
                    desc.format = UGIFormatToVk(_desc.depthStencil.format);
                    desc.samples = sampleCount;
                    desc.loadOp = LoadOpToVk(_desc.depthStencil.loadAction);
                    desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
                    desc.stencilLoadOp = LoadOpToVk(_desc.depthStencil.loadAction);
                    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                    desc.initialLayout = imageLayoutFromAccessType(_desc.depthStencil.initialAccessType);
                    desc.finalLayout = imageLayoutFromAccessType(_desc.depthStencil.finalAccessType);
                }
                attacmentDescriptions.push_back(desc);
            }
        }
        // setup color attachment reference -> attachment descriptions
        std::vector<VkAttachmentReference> colorAttachments; {
            VkAttachmentReference color;
            color.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            for (uint32_t i = 0; i < _desc.colorAttachmentCount; ++i) {
                color.attachment = i;
                colorAttachments.push_back(color);
            }
        }
        // setup depthstencil attachment reference -> attachment description
        VkAttachmentReference depthstencil = {
            _desc.colorAttachmentCount, // attachment index
            VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL // layout
        };
        // subpass description
        VkSubpassDescription subpassinfo; {
            subpassinfo.flags = 0;
            subpassinfo.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
            subpassinfo.pInputAttachments = nullptr;
            subpassinfo.inputAttachmentCount = 0;
            subpassinfo.pPreserveAttachments = nullptr;
            subpassinfo.preserveAttachmentCount = 0;
            subpassinfo.pResolveAttachments = nullptr;
            // color attachments
            subpassinfo.colorAttachmentCount = static_cast<uint32_t>(_desc.colorAttachmentCount);
            if (subpassinfo.colorAttachmentCount){
                subpassinfo.pColorAttachments = &colorAttachments[0];
            }
            // depth stencil
            if( _desc.depthStencil.format != UGIFormat::InvalidFormat) {
                subpassinfo.pDepthStencilAttachment = &depthstencil;
            } else {
                subpassinfo.pDepthStencilAttachment = nullptr;
            }
        }
        // subpass dependency
        VkSubpassDependency subpassDependency[3]; {
            subpassDependency[0].srcSubpass = VK_SUBPASS_EXTERNAL;
            subpassDependency[0].dstSubpass = 0;
            subpassDependency[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            subpassDependency[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpassDependency[0].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            subpassDependency[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            subpassDependency[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
            //
            subpassDependency[1].srcSubpass = 0;
            subpassDependency[1].dstSubpass = VK_SUBPASS_EXTERNAL;
            subpassDependency[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            subpassDependency[1].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            subpassDependency[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            subpassDependency[1].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
            subpassDependency[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

            subpassDependency[2].srcSubpass = 0;
			subpassDependency[2].dstSubpass = 0;
			subpassDependency[2].srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			subpassDependency[2].dstStageMask = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
			subpassDependency[2].srcAccessMask = 0;
			subpassDependency[2].dstAccessMask = VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;
			subpassDependency[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        }
        // render pass create info
        VkRenderPassCreateInfo rpinfo; {
            rpinfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            rpinfo.pNext = nullptr;
            rpinfo.flags = 0;
            rpinfo.pAttachments = &attacmentDescriptions[0];
            rpinfo.attachmentCount = static_cast<uint32_t>(attacmentDescriptions.size());
            rpinfo.subpassCount = 1;
            rpinfo.pSubpasses = &subpassinfo;
            rpinfo.dependencyCount = sizeof(subpassDependency)/sizeof(subpassDependency[0]);
            rpinfo.pDependencies = &subpassDependency[0];
        }
        // create render pass object
        VkRenderPass renderPass;
        VkResult rst = vkCreateRenderPass( _device->device(), &rpinfo, nullptr, &renderPass);
        if (rst != VK_SUCCESS) {
            return VK_NULL_HANDLE;
        }
        registRenderPass( _desc, renderPass, hash );
        return renderPass;
    }

    void RenderPassObjectManager::registRenderPass( const RenderPassDescription& _subpassDesc, VkRenderPass _renderPass, uint64_t& hash, uint32_t _subpassIndex  ) {
        hash = CompatibleHashFunc(_subpassDesc, _subpassIndex);
        m_compatibleTable[hash] = _renderPass;
    }

    VkRenderPass RenderPassObjectManager::queryCompatibleRenderPass( const RenderPassDescription& _subpassDesc, uint32_t _subpassIndex ) {
        uint64_t hash = CompatibleHashFunc(_subpassDesc, _subpassIndex);
        auto iter = m_compatibleTable.find(hash);
        if(iter!=m_compatibleTable.end()){
            return iter->second;
        }
        return VK_NULL_HANDLE;
    }
    
    IRenderPass* RenderPass::CreateRenderPass( Device* _device, const RenderPassDescription& _desc, Texture** _colors, Texture* _depthStencil ) {
        // 为什么要预先创建好纹理再传进来呢？其实是有原因的，因为RenderPass有一个自动转Layout的过程，所以它需要一个初始Layout一个最终Layout，我们需要告诉它，然后
        // 让它自动转，所以我们提供的纹理需要是指定的初始Layout，但是直接创建出来的纹理只能是通用或者未定义的布局，这个没办法和初始Layout完全对应
        // 所以需要我们传进来纹理布局是已经转换好的
        uint64_t hash;
        VkRenderPass renderPass = _device->renderPassObjectManager()->getRenderPass( _device, _desc, hash);
        //
        if( VK_NULL_HANDLE == renderPass ) {
            return nullptr;
        }
        RenderPass* rst = new RenderPass();
        rst->m_subpassHash = hash;
        rst->m_decription = _desc;
        rst->m_renderPass = renderPass;
        uint32_t width = 0, height = 0;
        if( _colors[0]) {
            width = _colors[0]->desc().width; height = _colors[0]->desc().height;
        } else {
            assert(_depthStencil);
            width = _depthStencil->desc().width; height = _depthStencil->desc().height;
        }
        //
        rst->m_size.width = width;
        rst->m_size.height = height;
        //
        uint32_t clearCount = 0;
        // 生成所需纹理
        // 计算清屏数量
        rst->m_colorTextureCount = 0;
        for (uint32_t i = 0; i < _desc.colorAttachmentCount; ++i) {
            rst->m_colorTextures[i] = _colors[i];
            ++rst->m_colorTextureCount;
            if (_desc.colorAttachments[i].loadAction == AttachmentLoadAction::Clear) {
                ++clearCount;
            }
        }
        if (_desc.depthStencil.format != UGIFormat::InvalidFormat) {
            rst->m_depthStencilTexture = _depthStencil;
            assert( rst->m_depthStencilTexture );
            if (_desc.depthStencil.loadAction == AttachmentLoadAction::Clear) {
                ++clearCount;
            }
        }
        // 创建 framebuffer
        rst->m_clearCount = clearCount;
        for (uint32_t i = 0; i < _desc.colorAttachmentCount; ++i) {
            rst->m_colorImageLayouts[i] = imageLayoutFromAccessType(_desc.colorAttachments[i].finalAccessType);
        }
        rst->m_depthStencilImageLayout = imageLayoutFromAccessType(_desc.depthStencil.finalAccessType);
        rst->m_framebuffer = VK_NULL_HANDLE;
        {
            uint32_t attachmentCount = 0;
            VkImageView imageViews[MaxRenderTarget + 1];
            for (uint32_t i = 0; i<_desc.colorAttachmentCount; ++i) {
                imageViews[attachmentCount] = rst->m_colorTextures[i]->imageView();
                ++attachmentCount;
            }
            if (_desc.depthStencil.format != UGIFormat::InvalidFormat) {
                imageViews[attachmentCount] = rst->m_depthStencilTexture->imageView();
                ++attachmentCount;
            }
            VkFramebufferCreateInfo fbinfo = {}; {
                fbinfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                fbinfo.pNext = nullptr;
                fbinfo.flags = 0;
                fbinfo.attachmentCount = attachmentCount;
                fbinfo.pAttachments = imageViews;
                fbinfo.width = width;
                fbinfo.height = height;
                fbinfo.layers = 1;
                fbinfo.renderPass = rst->m_renderPass;
            }
            if (VK_SUCCESS != vkCreateFramebuffer( _device->device(), &fbinfo, nullptr, &rst->m_framebuffer)) {
                return nullptr;
            }
        }
        rst->m_renderPassBeginInfo.pNext = nullptr;
        rst->m_renderPassBeginInfo.renderPass = rst->m_renderPass;
        rst->m_renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rst->m_renderPassBeginInfo.framebuffer = rst->m_framebuffer;
        rst->m_renderPassBeginInfo.pClearValues = rst->m_clearValues;
        rst->m_renderPassBeginInfo.clearValueCount = rst->m_depthStencilTexture? rst->m_colorTextureCount + 1: rst->m_colorTextureCount;
        rst->m_renderPassBeginInfo.renderArea.offset = { 0, 0 };
        rst->m_renderPassBeginInfo.renderArea.extent = { rst->m_size.width, rst->m_size.height };
        //
        return rst;
    }

    void RenderPass::setClearValues( const RenderPassClearValues& clearValues ) {
        for( uint32_t i = 0; i< m_colorTextureCount; ++i ) {
            m_clearValues[i].color.float32[0] = clearValues.colors[i].r;
            m_clearValues[i].color.float32[1] = clearValues.colors[i].g;
            m_clearValues[i].color.float32[2] = clearValues.colors[i].b;
            m_clearValues[i].color.float32[3] = clearValues.colors[i].a;
        }
        if( m_depthStencilTexture) {
            m_clearValues[m_colorTextureCount].depthStencil.depth = clearValues.depth;
            m_clearValues[m_colorTextureCount].depthStencil.stencil = clearValues.stencil;
        }
    }

    void RenderPass::begin( RenderCommandEncoder* encoder ) const {
        for( uint32_t i = 0; i<m_decription.colorAttachmentCount; ++i ) {
            ((ResourceCommandEncoder*)encoder)->imageTransitionBarrier( m_colorTextures[i], m_decription.colorAttachments[i].initialAccessType, PipelineStages::Bottom, StageAccess::Read, PipelineStages::ColorAttachmentOutput, StageAccess::Write );
        }
        if(m_depthStencilTexture) {
            ((ResourceCommandEncoder*)encoder)->imageTransitionBarrier( m_depthStencilTexture, m_decription.depthStencil.initialAccessType, PipelineStages::Bottom, StageAccess::Read, PipelineStages::EaryFragmentTestShading, StageAccess::Write);
        }
        //
        vkCmdBeginRenderPass( *encoder->commandBuffer(), &m_renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    }
    
    void RenderPass::end( RenderCommandEncoder* encoder ) const {
        vkCmdEndRenderPass(*encoder->commandBuffer());
        for( uint32_t i = 0; i<m_decription.colorAttachmentCount; ++i ) {
            m_colorTextures[i]->updateAccessType( m_decription.colorAttachments[i].finalAccessType );
        }
        if(m_depthStencilTexture) {
            m_depthStencilTexture->updateAccessType(m_decription.depthStencil.finalAccessType);
        }
    }

}