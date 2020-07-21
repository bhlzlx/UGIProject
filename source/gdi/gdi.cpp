#include "gdi.h"
#include <ugi/Device.h>
#include <ugi/CommandQueue.h>
#include <ugi/Buffer.h>
#include <ugi/commandBuffer.h>
#include <ugi/Drawable.h>
#include <ugi/Pipeline.h>
#include <ugi/Argument.h>
#include <hgl/assets/AssetsSource.h>

namespace ugi {
    namespace gdi {
        
        bool GDIContext::initialize()
        {
            hgl::io::InputStream* pipelineFile = _assetsSource->Open( hgl::UTF8String("/shaders/GDITest/pipeline.bin") );
            if (!pipelineFile) {
                return false;
            }
            auto pipelineFileSize = pipelineFile->GetSize();
            auto mem = malloc(pipelineFileSize);
            char* pipelineBuffer = (char*)mem;
            pipelineFile->ReadFully(pipelineBuffer, pipelineFileSize);
            PipelineDescription& pipelineDesc = *(PipelineDescription*)pipelineBuffer;
            pipelineBuffer += sizeof(PipelineDescription);
            for (auto& shader : pipelineDesc.shaders) {
                if (shader.spirvData) {
                    shader.spirvData = (uint64_t)pipelineBuffer;
                    pipelineBuffer += shader.spirvSize;
                }
            }
            pipelineDesc.pologonMode = PolygonMode::Fill;
            pipelineDesc.topologyMode = TopologyMode::TriangleList;
            pipelineDesc.renderState.cullMode = CullMode::None;
            pipelineDesc.renderState.blendState.enable = true;
            _pipeline = _device->createPipeline(pipelineDesc);
            _pipelineDesc = *(ugi::PipelineDescription*)mem;
            free(mem);
            if (!_pipeline) {
                return false;
            }
            //
            return true;
        }

        ugi::Pipeline* GDIContext::pipeline() const noexcept
        {
            return _pipeline;
        }

        const ugi::PipelineDescription& GDIContext::pipelineDescription() const noexcept {
            return _pipelineDesc;
        }

        void GDIContext::setSize(const hgl::Vector2f& size)
        {
            _size = size;
        }
    }
}