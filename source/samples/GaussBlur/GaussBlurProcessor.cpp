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
        _pipelineDescription = *(PipelineDescription*)pipelineBuffer;
        pipelineBuffer += sizeof(PipelineDescription);
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

    void GaussBlurProcessor::prepareResource( GaussBlurItem* item, ResourceCommandEncoder* encoder, UniformAllocator* uniformAllocator ) {
        // update uniform buffer
        auto argGroup = item->argumentGroup();
        ResourceDescriptor descriptor;
        descriptor.descriptorHandle = _parameterHandle;
        descriptor.type = ArgumentDescriptorType::UniformBuffer;
        uniformAllocator->allocateForDescriptor( descriptor, item->parameter());
        // prepare for descriptor set
        encoder->prepareArgumentGroup(argGroup);
    }

    void GaussBlurProcessor::processBlur( GaussBlurItem* item, ComputeCommandEncoder* encoder) {
        auto argGroup = item->argumentGroup();
        encoder->bindArgumentGroup(argGroup);
        encoder->dispatch(16, 16, 1);
        encoder->endEncode();
    }

    GaussBlurItem::GaussBlurItem( ArgumentGroup* group, Texture* texture0, Texture* texture1 )
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
        ResourceDescriptor resDesc;
        resDesc.texture = texture0;
        resDesc.descriptorHandle = _inputImageHandle;
        resDesc.type = ArgumentDescriptorType::StorageImage;
        argGroup->updateDescriptor(resDesc);
        resDesc.texture = texture1;
        resDesc.descriptorHandle = _outputImageHandle;
        argGroup->updateDescriptor(resDesc);
        //
        GaussBlurItem* item = new GaussBlurItem(argGroup, texture0, texture1 );
        return item;
    }

}