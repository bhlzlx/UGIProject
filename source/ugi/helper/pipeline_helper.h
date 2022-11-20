#pragma once
#include <LightWeightCommon/lightweight_comm.h>
#include <ugi/pipeline.h>
#include <vector>

namespace ugi {

    class PipelineHelper {
    private:
        pipeline_desc_t         desc_;
        uint8_t*                data_;
    public: 
        PipelineHelper(PipelineHelper const&) = delete;
        PipelineHelper(PipelineHelper &&) = delete;
        PipelineHelper()
            : desc_{}
            , data_(nullptr)
        {}
        pipeline_desc_t const& desc() const { return desc_; }
        static PipelineHelper FromIStream(comm::IStream* stream);
        ~PipelineHelper() {
            if(data_) {
                free(data_);
                data_ = nullptr;
            }
        }
    };

}
