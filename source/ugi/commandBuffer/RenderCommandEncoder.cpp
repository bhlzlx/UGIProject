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
#include <render_components/MeshPrimitive.h>
#include <vector>

namespace ugi {

    // void RenderCommandEncoder::drawIndexed( Drawable* drawable, uint32_t indexOffset, uint32_t indexCount, uint32_t vertexOffset, uint32_t instanceCount) {
    //     drawable->bind(_commandBuffer);
    //     vkCmdDrawIndexed( *_commandBuffer, indexCount, instanceCount, indexOffset, vertexOffset, 0);
    // }

    // void RenderCommandEncoder::draw( Drawable* drawable, uint32_t vertexCount, uint32_t baseVertexIndex) {
    //     drawable->bind(_commandBuffer);
    //     vkCmdDraw( *_commandBuffer, vertexCount, 1, baseVertexIndex, 0 );
    // }


    void RenderCommandEncoder::draw(Mesh const* mesh, uint32_t instanceCount) {
        VkCommandBuffer cmd = *_commandBuffer;
        auto alloc = mesh->buffer();
        VkDeviceSize offset = alloc.offset;
        vkCmdBindVertexBuffers(cmd, 0, 1, &alloc.buffer, &offset); // one vtx buffer only
        vkCmdBindIndexBuffer(cmd, alloc.buffer, offset + mesh->iboffset(), VkIndexType::VK_INDEX_TYPE_UINT16);
        vkCmdDrawIndexed(cmd, mesh->indexCount(), instanceCount, 0, 0, 0);
    }

    void RenderCommandEncoder::drawIndirect(Mesh const* meshes, uint32_t count) {
        VkCommandBuffer cmd = *_commandBuffer;
        vkCmdDrawIndexedIndirect(cmd, )
    }

    void RenderCommandEncoder::bindPipeline( GraphicsPipeline* pipeline ) {
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

    void RenderCommandEncoder::bindArgumentGroup( DescriptorBinder* argGroup ) {
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