#include "../CommandBuffer.h"
#include "../VulkanFunctionDeclare.h"
#include "../CommandBuffer.h"
#include "../UGIDeclare.h"
#include "../RenderPass.h"
#include "../Buffer.h"
#include "../Texture.h"
#include "../Pipeline.h"
#include "../Drawable.h"
#include "../UGITypeMapping.h"
#include "../Argument.h"
#include <vector>

namespace ugi {

    void RenderCommandEncoder::drawIndexed( Drawable* drawable, uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset ) {
        drawable->bind(_commandBuffer);
        vkCmdDrawIndexed( *_commandBuffer, indexCount, 1, indexOffset, vertexOffset, 0 );
    }

    void RenderCommandEncoder::draw( Drawable* drawable, uint32_t vertexCount, uint32_t baseVertexIndex) {
        drawable->bind(_commandBuffer);
        vkCmdDraw( *_commandBuffer, vertexCount, 1, baseVertexIndex, 0 );
    }

    void RenderCommandEncoder::bindPipeline( Pipeline* pipeline ) {
        pipeline->bind(this);
    }

    void RenderCommandEncoder::setViewport( float x, float y, float width, float height, float depthMin, float depthMax ) {
        _viewport.x = x;
        _viewport.y = y;
        _viewport.width = width;
        _viewport.height = height;
        _viewport.minDepth = depthMin;
        _viewport.maxDepth = depthMax;
        vkCmdSetViewport( *_commandBuffer, 0, 1, &_viewport );
    }

    void RenderCommandEncoder::setScissor( int x, int y, int width, int height ) {
        VkRect2D scissor;
        scissor.offset = { x, y };
        scissor.extent = { (uint32_t)width, (uint32_t)height };
        vkCmdSetScissor( *_commandBuffer, 0, 1, &scissor );
    }

    void RenderCommandEncoder::bindArgumentGroup( ArgumentGroup* argGroup ) {
        argGroup->bind(_commandBuffer );
    }

    void RenderCommandEncoder::setLineWidth( float lineWidth ) {
        vkCmdSetLineWidth( *_commandBuffer, lineWidth );
    }

    void RenderCommandEncoder::endEncode() {
        _renderPass->end(this);
        _commandBuffer = nullptr;
    }

}