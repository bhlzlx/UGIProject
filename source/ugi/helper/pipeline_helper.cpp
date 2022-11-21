#include "pipeline_helper.h"
#include <LightWeightCommon/io/archive.h>

namespace ugi {

    PipelineHelper PipelineHelper::FromIStream(comm::IStream* stream) {
        PipelineHelper rst;
        rst.data_ = (uint8_t*)malloc(stream->size());
        auto pipelineBuffer = rst.data_;
        stream->read(rst.data_,stream->size());
        pipeline_desc_t& pipelineDesc = *(pipeline_desc_t*)rst.data_;
        pipelineBuffer += sizeof(pipeline_desc_t);
        for( auto& shader : pipelineDesc.shaders ) {
            if( shader.spirvData) {
                shader.spirvData = (uint64_t)pipelineBuffer;
                pipelineBuffer += shader.spirvSize;
            }
        }
        rst.desc_ = pipelineDesc;
        return rst;
    }

    PipelineHelper::PipelineHelper(PipelineHelper && other) {
        desc_ = other.desc_;
        data_ = other.data_;
        other.data_ = nullptr;
    }

}