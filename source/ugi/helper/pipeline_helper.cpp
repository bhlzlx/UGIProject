#include "pipeline_helper.h"
#include <LightWeightCommon/io/archive.h>

namespace ugi {

    PipelineHelper PipelineHelper::FromIStream(comm::IStream* stream) {
        PipelineHelper rst;
        rst.data_.resize(stream->size());
        auto pipelineFileSize = stream->size();
        auto pipelineBuffer = (uint8_t*)rst.data_.data();
        stream->read(rst.data_.data(),stream->size());
        pipeline_desc_t& pipelineDesc = *(pipeline_desc_t*)rst.data_.data();
        pipelineBuffer += sizeof(pipeline_desc_t);
        for( auto& shader : pipelineDesc.shaders ) {
            if( shader.spirvData) {
                shader.spirvData = (uint64_t)pipelineBuffer;
                pipelineBuffer += shader.spirvSize;
            }
        }
        return rst;
    }

}