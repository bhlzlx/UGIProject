#include "gdi.h"
#include <ugi/device.h>
#include <ugi/command_queue.h>
#include <ugi/buffer.h>
#include <ugi/commandbuffer.h>
#include <ugi/Drawable.h>
#include <ugi/pipeline.h>
#include <ugi/descriptor_binder.h>
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
            pipeline_desc_t& pipelineDesc = *(pipeline_desc_t*)pipelineBuffer;
            pipelineBuffer += sizeof(pipeline_desc_t);
            for (auto& shader : pipelineDesc.shaders) {
                if (shader.spirvData) {
                    shader.spirvData = (uint64_t)pipelineBuffer;
                    pipelineBuffer += shader.spirvSize;
                }
            }
            pipelineDesc.pologonMode = polygon_mode_t::Fill;
            pipelineDesc.topologyMode = TopologyMode::TriangleList;
            pipelineDesc.renderState.cullMode = CullMode::None;
            pipelineDesc.renderState.blendState.enable = true;
            _pipeline = _device->createGraphicsPipeline(pipelineDesc);
            _pipelineDesc = *(ugi::pipeline_desc_t*)mem;
            free(mem);
            if (!_pipeline) {
                return false;
            }
            //
            return true;
        }

        ugi::GraphicsPipeline* GDIContext::pipeline() const
        {
            return _pipeline;
        }

        const ugi::pipeline_desc_t& GDIContext::pipelineDescription() const {
            return _pipelineDesc;
        }

        void GDIContext::setSize(const hgl::Vector2f& size)
        {
            _size = size;
            _scale = { _size.x / _standardSize.x, _size.y/_standardSize.y };
            _commonScale = _scale.x > _scale.y ? _scale.y : _scale.x;
        }
    }
}