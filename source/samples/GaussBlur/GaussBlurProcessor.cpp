#include "GaussBlurProcessor.h"

#include <ugi/commandBuffer/ComputeCommandEncoder.h>
#include <ugi/CommandBuffer.h>
#include <ugi/Device.h>
#include <ugi/Pipeline.h>
#include <ugi/Argument.h>
#include <ugi/UniformBuffer.h>
#include <hgl/assets/AssetsSource.h>


namespace ugi {

    GaussBlurProcessor::GaussBlurProcessor()
        : _device( nullptr )
        , _assetsSource( nullptr )
        , _pipeline( nullptr )
        , _pipelineDescription {}
        , _inputImageHandle(~0)
        , _outputImageHandle(~0)
        , _parameterHandle(~0)
    {
    }

    bool GaussBlurProcessor::intialize( ugi::Device* device, hgl::assets::AssetsSource* assetsSource ) {
        _device = device;
        _assetsSource = assetsSource;
        // Create Pipeline
        hgl::io::InputStream* pipelineFile = assetsSource->Open( hgl::UTF8String("/shaders/gaussBlur/pipeline.bin"));
        auto pipelineFileSize = pipelineFile->GetSize();
        char* pipelineBuffer = (char*)malloc(pipelineFileSize);
        pipelineFile->ReadFully(pipelineBuffer,pipelineFileSize);
        _pipelineDescription = *(pipeline_desc_t*)pipelineBuffer;
        pipelineBuffer += sizeof(pipeline_desc_t);
        for( auto& shader : _pipelineDescription.shaders ) {
            if( shader.spirvData) {
                shader.spirvData = (uint64_t)pipelineBuffer;
                pipelineBuffer += shader.spirvSize;
            }
        }
        _pipeline = _device->createComputePipeline(_pipelineDescription);
        pipelineFile->Close();
        if( !_pipeline) {
            return false;
        }
        return true;
    }

    void GaussBlurItem::setParameter( const GaussBlurParameter& parameter ) {
        _parameter = parameter;
    }

    void GaussBlurProcessor::processBlur( GaussBlurItem* item, CommandBuffer* commandBuffer, UniformAllocator* uniformAllocator ) {
        // update uniform buffer
        auto resEncoder = commandBuffer->resourceCommandEncoder();
        auto argGroup = item->argumentGroup();
        res_descriptor_t descriptor;
        descriptor.descriptorHandle = _parameterHandle;
        descriptor.bufferRange = sizeof(GaussBlurParameter);
        descriptor.type = res_descriptor_type::UniformBuffer;
        uniformAllocator->allocateForDescriptor( descriptor, item->parameter());
		argGroup->updateDescriptor(descriptor);
        // prepare for descriptor set
        resEncoder->prepareArgumentGroup(argGroup);

        resEncoder->imageTransitionBarrier( item->source(), ResourceAccessType::ShaderRead, PipelineStages::Bottom, StageAccess::Write, PipelineStages::Top, StageAccess::Read);
        resEncoder->imageTransitionBarrier( item->target(), ResourceAccessType::ShaderWrite, PipelineStages::Bottom, StageAccess::Read, PipelineStages::Top, StageAccess::Write);

        resEncoder->endEncode();
        //
        auto computeEncoder = commandBuffer->computeCommandEncoder();
		computeEncoder->bindPipeline(_pipeline);
        computeEncoder->bindArgumentGroup(argGroup);
        computeEncoder->dispatch(32, 32, 1);
        computeEncoder->endEncode();
    }

    GaussBlurItem::GaussBlurItem( DescriptorBinder* group, Texture* texture0, Texture* texture1 )
        : _argGroup( group )
        , _texture0( texture0 )
        , _texture1( texture1 )
    {
    }

    GaussBlurItem* GaussBlurProcessor::createGaussBlurItem( ugi::Texture* texture0, ugi::Texture* texture1 ) {
        auto argGroup = _pipeline->createArgumentGroup();
        if(_inputImageHandle == ~(0)) {
            _inputImageHandle = argGroup->GetDescriptorHandle("InputImage", _pipelineDescription);
            _outputImageHandle = argGroup->GetDescriptorHandle("OutputImage", _pipelineDescription);
            _parameterHandle = argGroup->GetDescriptorHandle("BlurParameter", _pipelineDescription);
            assert( _inputImageHandle!=~0 && _outputImageHandle!=~0 && _parameterHandle!=~0 );
        }
        res_descriptor_t resDesc;
        resDesc.texture = texture0;
        resDesc.descriptorHandle = _inputImageHandle;
        resDesc.type = res_descriptor_type::StorageImage;
        argGroup->updateDescriptor(resDesc);
        resDesc.texture = texture1;
        resDesc.descriptorHandle = _outputImageHandle;
        argGroup->updateDescriptor(resDesc);
        //
        GaussBlurItem* item = new GaussBlurItem(argGroup, texture0, texture1 );
        return item;
    }

}