#pragma once
#include <LightWeightCommon/lightweight_comm.h>
#include <ugi/pipeline.h>
#include <vector>

namespace ugi {

    class PipelineHelper {
    private:
        pipeline_desc_t         desc_;
        std::vector<uint8_t>    data_;
    public: 
        PipelineHelper()
            : desc_{}
            , data_{}
        {}
        pipeline_desc_t const& desc() const { return desc_; }
        static PipelineHelper FromIStream(comm::IStream* stream);
    };

}
