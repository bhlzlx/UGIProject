#pragma once
#include "ugi_types.h"
#include "ugi_declare.h"
#include "ugi_vulkan_private.h"
#include "vulkan_declare.h"
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
        uint64_t CompatibleHashFunc( const renderpass_desc_t& _subpassDesc, uint32_t _subpassIndex );
    private:        
        /*-------------------------------------------------------
        |  hash( attachment & subpass index ) <-> render pass   |
        ------------------------------------------------------- */
        // 也就是说可能存在多个hash值对应同一个renderpass，这些`hash`对应的是同一个`render pass`上不同的`subpass`
        // 但这种情况比较少，即一个`renderpass` 有多个 `subpass`
        std::unordered_map< uint64_t, VkRenderPass > _compatibleTable; ///< `key`是`subpass`的`hash`, 查询兼容`renderpass`对象的时候需要用它来查询
        // 配的 renderpass
        std::set<VkRenderPass> _renderPasses; ///< 所有已经分配的`render pass`
        //
        void registRenderPass( const renderpass_desc_t& _subpassDesc, VkRenderPass _renderPass, uint64_t& hash, uint32_t _subpassIndex = 0);
    public:
        RenderPassObjectManager()
            : _compatibleTable {}
        {
        }

        VkRenderPass getRenderPass( Device* _device,  const renderpass_desc_t& _subpassDesc, uint64_t& hash );
        VkRenderPass queryCompatibleRenderPass( const renderpass_desc_t& _subpassDesc, uint32_t _subpassIndex = 0);
    };

    // 
    // 最最常见的RenderPass，可能有一个或者多个RT
    // 由一个Framebuffer和一个Subpass 组成

    class IRenderPass {
    public:
        virtual void setClearValues( const renderpass_clearval_t& clearValues ) = 0;
        virtual uint32_t subpassCount() const = 0;
        virtual uint32_t subpassColorAttachmentCount( uint32_t subpassIndex ) const = 0;
        virtual uint64_t subpassHash( uint32_t subpassIndex ) const = 0;
        virtual VkRenderPass renderPass() const = 0;
        virtual void begin( RenderCommandEncoder* encoder ) const = 0;
        virtual void end( RenderCommandEncoder* encoder ) const = 0;
        virtual void release(Device* device) = 0;
        virtual image_view_t colorView(uint32_t index) = 0;
        virtual Texture* colorTexture(uint32_t index) = 0;
    };

    class RenderPass : IRenderPass {
        friend class CommandBuffer;
    private:
        renderpass_desc_t                           _decription;
        uint64_t                                    _subpassHash;                              ///> render pass's hash value
        //
        VkFramebuffer                               _framebuffer;                              ///> framebuffer 对象
        VkRenderPass                                _renderPass;                               // renderpass - 含一个subpass
        //
        uint32_t                                    _colorTextureCount;                        // RT数量
        Texture*                                    _colorTexture[MaxRenderTarget];
        image_view_t                                _colorViews[MaxRenderTarget];           // RT所使用的纹理对象
        Texture*                                    _dsTexture;
        image_view_t                                _dsView;;                                   // 深度和模板所使用的纹理对象

        VkImageLayout                               _colorImageLayouts[MaxRenderTarget];
        VkImageLayout                               _depthStencilImageLayout;
                //
        uint8_t                                     _clearCount;                               // render pass  切换初始状态时需要clear的数量
        VkClearValue                                _clearValues[MaxRenderTarget+1];           // color & depth-stencil clear values
                //
        VkRenderPassBeginInfo                       _renderPassBeginInfo;
                //
        extent_2d_t<uint32_t>                       _size; // 宽高
    public:
        virtual void setClearValues( const renderpass_clearval_t& clearValues ) override;
        //== 
        virtual uint32_t subpassCount() const override {
            return 1;
        }
        virtual uint64_t subpassHash( uint32_t subpassIndex ) const override {
            return _subpassHash;
        }
        virtual uint32_t subpassColorAttachmentCount( uint32_t subpassIndex ) const override {
            return _colorTextureCount;
        }
        virtual VkRenderPass renderPass() const override {
            return _renderPass;
        }
        virtual image_view_t colorView( uint32_t index ) override {
            if(index<_colorTextureCount) {
                return _colorViews[index];
            }
            return image_view_t();
        }
        virtual Texture* colorTexture(uint32_t index) override {
            if(index<_colorTextureCount) {
                return _colorTexture[index];
            }
            return nullptr;
        }
        virtual ~RenderPass() {};
        virtual void begin( RenderCommandEncoder* encoder ) const override;
        virtual void end( RenderCommandEncoder* encoder ) const override;
        virtual void release( Device* device ) override;
        //
        static IRenderPass* CreateRenderPass( Device* _device, const renderpass_desc_t& _desc, Texture** colors, Texture* depth, image_view_param_t const* colorView, image_view_param_t depthView);
    };

}