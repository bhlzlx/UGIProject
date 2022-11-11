#pragma once

#include "../UGIDeclare.h"
#include <cstdint>
#include <ugi/VulkanDeclare.h>

namespace ugi {

    class RenderCommandEncoder {
    private:
        CommandBuffer*                                  _commandBuffer;
        IRenderPass*                                    _renderPass;         ///> renderpass
        uint32_t                                        _subpass;            ///> subpass 的索引
        VkViewport                                      _viewport;
        VkRect2D                                        _scissor;
        GraphicsPipeline*                               _pipeline;
    public:
        RenderCommandEncoder( CommandBuffer* commandBuffer = nullptr, IRenderPass* renderPass  = nullptr ) 
            : _commandBuffer( commandBuffer )
            , _renderPass( renderPass )
            , _subpass( 0 )
        {
        }
        void bindPipeline( GraphicsPipeline* pipeline );
        void bindArgumentGroup( DescriptorBinder* argumentGroup );
        void setViewport( float x, float y, float width ,float height, float minDepth, float maxDepth );
        void setScissor( int x, int y, int width, int height );
        void setLineWidth( float lineWidth );
        void draw(Mesh const* mesh, uint32_t instanceCount);
        void drawIndirect(Mesh const* meshes, uint32_t count);
        // void drawIndexed( Drawable* drawable, uint32_t offset, uint32_t indexCount, uint32_t vertexOffset = 0, uint32_t instanceCount = 1);
        void nextSubpass();
        void endEncode();
        //
        const IRenderPass* renderPass() const {
            return _renderPass;
        }
        uint32_t subpass() const {
            return _subpass;
        }
        const CommandBuffer* commandBuffer() const {
            return _commandBuffer;
        }
    };

}