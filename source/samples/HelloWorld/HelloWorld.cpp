#include "HelloWorld.h"
#include <cassert>
#include <ugi/Device.h>
#include <ugi/Swapchain.h>
#include <ugi/CommandQueue.h>
#include <ugi/CommandBuffer.h>
#include <ugi/Buffer.h>
#include <ugi/RenderPass.h>
#include <ugi/Semaphore.h>
#include <ugi/Pipeline.h>
#include <ugi/Argument.h>
#include <ugi/Texture.h>
#include <ugi/Drawable.h>
#include <ugi/UniformBuffer.h>
#include <ugi/Descriptor.h>
// #include <hgl/assets/AssetsSource.h>

#include <cmath>

namespace ugi {

    bool RenderContext::initialize(void* wnd, ugi::DeviceDescriptor deviceDesc, common::IArchive* archive) {
        renderSystem = new RenderSystem();
        device = renderSystem->createDevice(deviceDesc, archive);
        uniformAllocator = device->createUniformAllocator();
        descriptorSetAllocator = device->descriptorSetAllocator();
        swapchain = device->createSwapchain(wnd);
        graphicsQueue = device->graphicsQueues()[0];
        uploadQueue = device->transferQueues()[0];
        for( size_t i = 0; i<MaxFlightCount; ++i) {
            frameCompleteFences[i] = device->createFence();
            renderCompleteSemaphores[i] = device->createSemaphore();
            commandBuffers[i] = graphicsQueue->createCommandBuffer(device);
        }
        return true;
    }

    bool HelloWorld::initialize( void* _wnd, common::IArchive* arch) {
        auto pipelineFile = arch->openIStream("/shaders/triangle/pipeline.bin", {common::ReadFlag::binary});
        auto pipelineFileSize = pipelineFile->size();
        char* pipelineBuffer = (char*)malloc(pipelineFileSize);
        pipelineFile->read(pipelineBuffer,pipelineFile->size());
        pipeline_desc_t& pipelineDesc = *(pipeline_desc_t*)pipelineBuffer;
        pipelineBuffer += sizeof(pipeline_desc_t);
        for( auto& shader : pipelineDesc.shaders ) {
            if( shader.spirvData) {
                shader.spirvData = (uint64_t)pipelineBuffer;
                pipelineBuffer += shader.spirvSize;
            }
        }
        pipelineDesc.pologonMode = polygon_mode_t::Fill;
        pipelineDesc.topologyMode = topology_mode_t::TriangleList;
        printf("initialize\n");

        ugi::DeviceDescriptor descriptor; {
            descriptor.apiType = ugi::GRAPHICS_API_TYPE::VULKAN;
            descriptor.deviceType = ugi::GRAPHICS_DEVICE_TYPE::DISCRETE;
            descriptor.debugLayer = 1;
            descriptor.graphicsQueueCount = 1;
            descriptor.transferQueueCount = 1;
            descriptor.wnd = _wnd;
        }
        _renderContext.initialize(_wnd, descriptor, arch);
        pipelineDesc.renderState.cullMode = CullMode::None;
        pipelineDesc.renderState.blendState.enable = false;
        _pipeline = _renderContext.device->createGraphicsPipeline(pipelineDesc);
        //
        auto argGroup = _pipeline->argumentBinder();
        _vertexBuffer = _renderContext.device->createBuffer( BufferType::VertexBuffer, sizeof(float) * 5 * 3 );
        Buffer* vertexStagingBuffer = _renderContext.device->createBuffer( BufferType::StagingBuffer, sizeof(float) * 5 * 3 );
        float vertexData[] = {
            -0.5, -0.5, 0, 0, 0,
            0, 0.5, 0, 0.5, 1.0,
            0.5, -0.5, 0, 1.0, 0
        };
        void* ptr = vertexStagingBuffer->map( _renderContext.device );
        memcpy(ptr, vertexData, sizeof(vertexData));
        vertexStagingBuffer->unmap(_renderContext.device);

        uint16_t indexData[] = {
            0, 1, 2
        };
        _indexBuffer = _renderContext.device->createBuffer( BufferType::IndexBuffer, sizeof(indexData));
        Buffer* indexStagingBuffer = _renderContext.device->createBuffer( BufferType::StagingBuffer, sizeof(indexData) );
        ptr = indexStagingBuffer->map( _renderContext.device );
        memcpy(ptr, indexData, sizeof(indexData));
        indexStagingBuffer->unmap(_renderContext.device);
        auto updateCmd = _renderContext.uploadQueue->createCommandBuffer( _renderContext.device ); {
            updateCmd->beginEncode();
            auto resourceEncoder = updateCmd->resourceCommandEncoder();
            BufferSubResource subRes;
            subRes.offset = 0; subRes.size = _vertexBuffer->size();
            resourceEncoder->updateBuffer( _vertexBuffer, vertexStagingBuffer, &subRes, &subRes );
            subRes.size = sizeof(indexData);
            resourceEncoder->updateBuffer( _indexBuffer, indexStagingBuffer, &subRes, &subRes );

            _drawable = _renderContext.device->createDrawable(pipelineDesc);
            _drawable->setVertexBuffer( _vertexBuffer, 0, 0 );
            _drawable->setVertexBuffer( _vertexBuffer, 1, 12 );
            _drawable->setIndexBuffer( _indexBuffer, 0 );
            //
            tex_desc_t texDesc;
            texDesc.format = UGIFormat::RGBA8888_UNORM;
            texDesc.depth = 1;
            texDesc.width = 16;
            texDesc.height = 16;
            texDesc.type = TextureType::Texture2D;
            texDesc.mipmapLevel = 1;
            texDesc.arrayLayers = 1;
            _texture = _renderContext.device->createTexture(texDesc, ResourceAccessType::ShaderRead );
            image_view_param_t ivp;
            _imageView = _texture->createImageView(_renderContext.device, ivp);
            uint32_t texData[] = {
                0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
                0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
                0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
                0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
                0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
                0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
                0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 
                0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 0xff000000, 0xffffffff, 
            };
            auto texStagingBuffer =  _renderContext.device->createBuffer( BufferType::StagingBuffer, sizeof(texData) );
            ptr = texStagingBuffer->map(_renderContext.device);
            memcpy(ptr, texData, sizeof(texData));
            texStagingBuffer->unmap(_renderContext.device);
            
            //ImageSubResource texSubRes;
            ImageRegion region;
            region.offset = {};
            region.mipLevel = 0;
            region.arrayIndex = 0;
            region.arrayCount = 1;
            region.extent.height = region.extent.width = 8;
            region.extent.depth = 1;

            uint32_t offset = 0;
            resourceEncoder->updateImage( _texture, texStagingBuffer, &region, &offset, 1);
            region.offset.x = 8;
            resourceEncoder->updateImage( _texture, texStagingBuffer, &region, &offset, 1);
            region.offset.x = 0; region.offset.y = 8;
            resourceEncoder->updateImage( _texture, texStagingBuffer, &region, &offset, 1);
            region.offset.x = 8; region.offset.y = 8;
            resourceEncoder->updateImage( _texture, texStagingBuffer, &region, &offset, 1);
            resourceEncoder->endEncode();

            updateCmd->endEncode();
        }
        //
		Semaphore* imageAvailSemaphore = _renderContext.swapchain->imageAvailSemaphore();
		QueueSubmitInfo submitInfo {
			&updateCmd,
			1,
			nullptr,// submitInfo.semaphoresToWait
			0,
			nullptr, // submitInfo.semaphoresToSignal
			0
		};
		QueueSubmitBatchInfo submitBatch(&submitInfo, 1, _renderContext.frameCompleteFences[_flightIndex]);
        _renderContext.uploadQueue->submitCommandBuffers(submitBatch);
        _renderContext.uploadQueue->waitIdle();
        //
        res_descriptor_info_t argDescInfo = {};
        _uniformDescriptor.descriptorHandle = DescriptorBinder::GetDescriptorHandle("Argument1", pipelineDesc, &argDescInfo);
        _uniformDescriptor.type = argDescInfo.type;
        _uniformDescriptor.res.buffer.size = argDescInfo.dataSize;
        // 
        res_descriptor_t res;
        res.type = res_descriptor_type::Sampler;
        res.res.samplerState = _samplerState;
        res.descriptorHandle = DescriptorBinder::GetDescriptorHandle("triSampler", pipelineDesc, &argDescInfo );
        argGroup->updateDescriptor(res);
        
        res.type = res_descriptor_type::Image;
        res.res.imageView = _imageView.handle;;
        res.descriptorHandle = DescriptorBinder::GetDescriptorHandle("triTexture", pipelineDesc, &argDescInfo );
        //
        argGroup->updateDescriptor(res);
        //
        _flightIndex = 0;
        return true;
    }

    void HelloWorld::tick() {
        
        _renderContext.device->waitForFence( _renderContext.frameCompleteFences[_flightIndex] );
        _renderContext.descriptorSetAllocator->tick();
        _renderContext.uniformAllocator->tick();
        auto argGroup = _pipeline->argumentBinder();
        uint32_t imageIndex = _renderContext.swapchain->acquireNextImage(_renderContext.device, _flightIndex);
        //
        IRenderPass* mainRenderPass = _renderContext.swapchain->renderPass(imageIndex);
        auto cmdbuf = _renderContext.commandBuffers[_flightIndex];
        cmdbuf->beginEncode(); {
            auto resourceEncoder = cmdbuf->resourceCommandEncoder();
            resourceEncoder->imageTransitionBarrier(_texture, ugi::ResourceAccessType::ShaderRead, ugi::PipelineStages::Transfer, ugi::StageAccess::Write, ugi::PipelineStages::FragmentShading, ugi::StageAccess::Read);
            resourceEncoder->endEncode();
            static uint64_t angle = 0;
            ++angle;
            float sinVal = sin( ((float)angle)/180.0f * 3.1415926f );
            float cosVal = cos( ((float)angle)/180.0f * 3.1415926f );

            float col2[2][4] = {
                { cosVal, -sinVal, 0, 0 },
                { sinVal,  cosVal, 0, 0 }
            };

            static ugi::raster_state_t rasterizationState;
            if( angle % 180 == 0 ) {
                rasterizationState.polygonMode = polygon_mode_t::Fill;
            } else if( angle % 90 == 0) {
                rasterizationState.polygonMode = polygon_mode_t::Line;
            }
            auto ubo = _renderContext.uniformAllocator->allocate(sizeof(col2));
            ubo.writeData(0, &col2, sizeof(col2));
            _uniformDescriptor.res.buffer.offset = ubo.offset();
            _uniformDescriptor.res.buffer.buffer = (size_t)ubo.buffer()->buffer(); 
            argGroup->updateDescriptor(_uniformDescriptor);

            // auto resourceEncoder = cmdbuf->resourceCommandEncoder();
            // // resourceEncoder->imageTransitionBarrier(_imageResources[i].texture(), ResourceAccessType::ShaderRead, PipelineStages::Bottom, StageAccess::Write, PipelineStages::VertexInput, StageAccess::Read);
            // resourceEncoder->prepareArgumentGroup(argGroup);
            // resourceEncoder->endEncode();
            //
            renderpass_clearvalue_t clearValues;
            clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
            clearValues.depth = 1.0f;
            clearValues.stencil = 0xffffffff;

            mainRenderPass->setClearValues(clearValues);

            auto renderCommandEncoder = cmdbuf->renderCommandEncoder( mainRenderPass ); {
                renderCommandEncoder->setLineWidth(1.0f);
                _pipeline->setRasterizationState(rasterizationState);
                renderCommandEncoder->setViewport(0, 0, _width, _height, 0, 1.0f );
                renderCommandEncoder->setScissor( 0, 0, _width, _height );
                renderCommandEncoder->bindPipeline(_pipeline);
                renderCommandEncoder->bindArgumentGroup(argGroup);
                // renderCommandEncoder->drawIndexed( _drawable, 0, 3 );
                renderCommandEncoder->draw( _drawable, 3, 0 );
            }
            renderCommandEncoder->endEncode();
        }
        cmdbuf->endEncode();

		Semaphore* imageAvailSemaphore = _renderContext.swapchain->imageAvailSemaphore();
		QueueSubmitInfo submitInfo {
			&cmdbuf,
			1,
			&imageAvailSemaphore,// submitInfo.semaphoresToWait
			1,
			&_renderContext.renderCompleteSemaphores[_flightIndex], // submitInfo.semaphoresToSignal
			1
		};
		QueueSubmitBatchInfo submitBatch(&submitInfo, 1, _renderContext.frameCompleteFences[_flightIndex]);

        bool submitRst = _renderContext.graphicsQueue->submitCommandBuffers(submitBatch);
        assert(submitRst);
        _renderContext.swapchain->present( _renderContext.device, _renderContext.graphicsQueue, _renderContext.renderCompleteSemaphores[_flightIndex] );

    }
        
    void HelloWorld::resize(uint32_t width, uint32_t height) {
        _renderContext.swapchain->resize( _renderContext.device, width, height );
        //
        _width = width;
        _height = height;
    }

    void HelloWorld::release() {
    }

    const char * HelloWorld::title() {
        return "HelloWorld";
    }
        
    uint32_t HelloWorld::rendererType() {
        return 0;
    }

}

ugi::HelloWorld theapp;

UGIApplication* GetApplication() {
    return &theapp;
}