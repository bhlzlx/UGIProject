#pragma once
#include "UGITypes.h"
#include "UGIDeclare.h"
#include "VulkanDeclare.h"
#include <array>
#include <unordered_map>
#include <set>
#include <stack>

namespace ugi {

    /**
     * @brief Render Pass Object cache manager
     * 类的详细概述
     */
    class RenderPassObjectManager {
    private:
        uint64_t CompatibleHashFunc( const RenderPassDescription& _subpassDesc, uint32_t _subpassIndex );
    private:        
        /*-------------------------------------------------------
        |  hash( attachment & subpass index ) <-> render pass   |
        ------------------------------------------------------- */
        // 也就是说可能存在多个hash值对应同一个renderpass，这些`hash`对应的是同一个`render pass`上不同的`subpass`
        // 但这种情况比较少，即一个`renderpass` 有多个 `subpass`
        std::unordered_map< uint64_t, VkRenderPass > m_compatibleTable; ///< `key`是`subpass`的`hash`, 查询兼容`renderpass`对象的时候需要用它来查询
        // 配的 renderpass
        std::set<VkRenderPass> m_renderPasses; ///< 所有已经分配的`render pass`
        //
        void registRenderPass( const RenderPassDescription& _subpassDesc, VkRenderPass _renderPass, uint64_t& hash, uint32_t _subpassIndex = 0);
    public:
        RenderPassObjectManager()
            : m_compatibleTable {}
        {
        }

        VkRenderPass getRenderPass( Device* _device,  const RenderPassDescription& _subpassDesc, uint64_t& hash );
        VkRenderPass queryCompatibleRenderPass( const RenderPassDescription& _subpassDesc, uint32_t _subpassIndex = 0);
    };

    // 
    // 最最常见的RenderPass，可能有一个或者多个RT
    // 由一个Framebuffer和一个Subpass 组成

    class IRenderPass {
    public:
        virtual void setClearValues( const RenderPassClearValues& clearValues ) = 0;
        virtual uint32_t subpassCount() const = 0;
        virtual uint32_t subpassColorAttachmentCount( uint32_t subpassIndex ) const = 0;
        virtual uint64_t subpassHash( uint32_t subpassIndex ) const = 0;
        virtual VkRenderPass renderPass() const = 0;
        virtual void begin( RenderCommandEncoder* encoder ) const = 0;
        virtual void end( RenderCommandEncoder* encoder ) const = 0;
    };

    class RenderPass : IRenderPass {
        friend class CommandBuffer;
    private:
        RenderPassDescription                       m_decription;
        uint64_t                                    m_subpassHash;                              ///> render pass's hash value
        //
        VkFramebuffer                               m_framebuffer;                              ///> framebuffer 对象
        VkRenderPass                                m_renderPass;                               // renderpass - 含一个subpass
        //
        Texture*                                    m_colorTextures[MaxRenderTarget];           // RT所使用的纹理对象
        uint32_t                                    m_colorTextureCount;                        // RT数量
        Texture*                                    m_depthStencilTexture;                      // 深度和模板所使用的纹理对象

        VkImageLayout                               m_colorImageLayouts[MaxRenderTarget];
        VkImageLayout                               m_depthStencilImageLayout;
                //
        uint8_t                                     m_clearCount;                               // render pass  切换初始状态时需要clear的数量
        VkClearValue                                m_clearValues[MaxRenderTarget+1];           // color & depth-stencil clear values
                //
        VkRenderPassBeginInfo                       m_renderPassBeginInfo;
                //
        Size<uint32_t>                              m_size; // 宽高
    public:
        virtual void setClearValues( const RenderPassClearValues& clearValues ) override;
        //== 
        virtual uint32_t subpassCount() const override {
            return 1;
        }
        virtual uint64_t subpassHash( uint32_t subpassIndex ) const override {
            return m_subpassHash;
        }
        virtual uint32_t subpassColorAttachmentCount( uint32_t subpassIndex ) const override {
            return m_colorTextureCount;
        }
        virtual VkRenderPass renderPass() const override {
            return m_renderPass;
        }
        virtual void begin( RenderCommandEncoder* encoder ) const override;
        virtual void end( RenderCommandEncoder* encoder ) const override;
        //
        static IRenderPass* CreateRenderPass( Device* _device, const RenderPassDescription& _renderPass, Texture** _colors, Texture* _depthStencil );
    };

    class OptimizedRenderPass {

    };

}