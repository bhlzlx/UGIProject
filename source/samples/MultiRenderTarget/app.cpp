#include "app.h"
#include <cassert>
#include <ugi/device.h>
#include <ugi/swapchain.h>
#include <ugi/command_queue.h>
#include <ugi/command_buffer.h>
#include <ugi/buffer.h>
#include <ugi/render_pass.h>
#include <ugi/Semaphore.h>
#include <ugi/pipeline.h>
#include <ugi/descriptor_binder.h>
#include <ugi/texture.h>
#include <ugi/Drawable.h>
#include <ugi/uniform_buffer_allocator.h>
#include <kwheel/base/io/archive.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace ugi {

    bool App::initialize( void* _wnd, kwheel::IArchive* _archive) {

        auto pipelineFile = _archive->open(
            "/shaders/sphere/pipeline.bin",
            (uint32_t)kwheel::OpenFlagBits::Binary | 
            (uint32_t)kwheel::OpenFlagBits::Read,
            1
            );
        assert( pipelineFile );

        const uint8_t* basePtr = (const uint8_t*)pipelineFile->constData();
        basePtr += sizeof(ugi::PipelineDescription);
        PipelineDescription pipelineDesc;
        pipelineFile->read(sizeof(pipelineDesc), &pipelineDesc);
        for( auto& shader : pipelineDesc.shaders ) {
            if( shader.spirvData) {
                shader.spirvData = (uint32_t*)(basePtr);
                basePtr += shader.spirvSize;
            }
        }
        
        pipelineDesc.pologonMode = PolygonMode::Fill;
        //
        pipelineDesc.topologyMode = TopologyMode::TriangleList;

        printf("initialize\n");

        _renderSystem = new ugi::RenderSystem();

        ugi::device_descriptor_t descriptor; {
            descriptor.apiType = ugi::GraphicsAPIType::VULKAN;
            descriptor.deviceType = ugi::GraphicsDeviceType::DISCRETE;
            descriptor.debugLayer = 1;
            descriptor.graphicsQueueCount = 1;
            descriptor.transferQueueCount = 1;
            descriptor.wnd = _wnd;
        }
        _device = _renderSystem->createDevice(descriptor, _archive);
        _uniformAllocator = _device->createUniformAllocator();
        _swapchain = _device->createSwapchain( _wnd );
        //
        // command queues
        _graphicsQueue = _device->graphicsQueues()[0];
        _uploadQueue = _device->transferQueues()[0];
        //
        for( size_t i = 0; i<MaxFlightCount; ++i) {
            _frameCompleteFences[i] = _device->createFence();
            _renderCompleteSemaphores[i] = _device->createSemaphore();
            _commandBuffers[i] = _graphicsQueue->createCommandBuffer( _device );
        }
        pipelineDesc.renderState.cullMode = CullMode::None;
        pipelineDesc.renderState.blendState.enable = false;
        _pipeline = _device->createGraphicsPipeline(pipelineDesc);
        //
        _argumentGroup = _pipeline->createArgumentGroup();

        std::vector<CVertex>        vertices;
        std::vector<uint16_t>       indices;
        CreateSphere(8, 12, vertices, indices);

        m_vertexBuffer = _device->createBuffer( BufferType::VertexBuffer, sizeof(CVertex) * vertices.size() );
        m_indexBuffer = _device->createBuffer( BufferType::IndexBuffer, sizeof(uint16_t) * indices.size() );
        Buffer* vertexStagingBuffer = _device->createBuffer( BufferType::StagingBuffer, m_vertexBuffer->size() );
        Buffer* indexStagingBuffer = _device->createBuffer( BufferType::StagingBuffer, m_indexBuffer->size() );

        void* ptr = vertexStagingBuffer->map( _device );
        memcpy(ptr, vertices.data(), m_vertexBuffer->size());
        vertexStagingBuffer->unmap(_device);

        ptr = indexStagingBuffer->map( _device );
        memcpy(ptr, indices.data(), m_indexBuffer->size());
        indexStagingBuffer->unmap(_device);

        auto updateCmd = _uploadQueue->createCommandBuffer( _device );

        updateCmd->beginEncode();

        auto resourceEncoder = updateCmd->resourceCommandEncoder();

        buffer_subres_t subRes;
        subRes.offset = 0; subRes.size = m_vertexBuffer->size();
        resourceEncoder->updateBuffer( m_vertexBuffer, vertexStagingBuffer, &subRes, &subRes );
        subRes.size = m_indexBuffer->size();
        resourceEncoder->updateBuffer( m_indexBuffer, indexStagingBuffer, &subRes, &subRes );

        m_drawable = _device->createDrawable(pipelineDesc);
        m_drawable->setVertexBuffer( m_vertexBuffer, 0, 0 );
        m_drawable->setIndexBuffer( m_indexBuffer, 0 );
        //
        tex_desc_t texDesc;
        texDesc.format = UGIFormat::RGBA8888_UNORM;
        texDesc.depth = 1;
        texDesc.width = 16;
        texDesc.height = 16;
        texDesc.type = TextureType::Texture2D;
        texDesc.mipmapLevel = 1;
        texDesc.layoutCount = 1;
        _texture = _device->createTexture(texDesc, ResourceAccessType::ShaderRead );

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
        auto texStagingBuffer =  _device->createBuffer( BufferType::StagingBuffer, sizeof(texData) );
        ptr = texStagingBuffer->map(_device);
        memcpy(ptr, texData, sizeof(texData));
        texStagingBuffer->unmap(_device);

        image_subres_t texSubRes;
        texSubRes.baseLayer = 0;
        texSubRes.offset = { 0, 0, 0 };
        texSubRes.size.depth = 1;
        texSubRes.size.width = texSubRes.size.height = 8;
        texSubRes.mipLevelCount = 1;
        texSubRes.baseMipLevel = 0;
        texSubRes.layerCount = 1;
        subRes.size = sizeof(texData);

        resourceEncoder->updateImage( _texture, texStagingBuffer, &texSubRes, &subRes );
        texSubRes.offset.x = 8;
        resourceEncoder->updateImage( _texture, texStagingBuffer, &texSubRes, &subRes );
        texSubRes.offset.y = 8; texSubRes.offset.x = 0;
        resourceEncoder->updateImage( _texture, texStagingBuffer, &texSubRes, &subRes );
        texSubRes.offset.y = 8; texSubRes.offset.x = 8;
        resourceEncoder->updateImage( _texture, texStagingBuffer, &texSubRes, &subRes );

        resourceEncoder->endEncode();

        updateCmd->endEncode();
        //
        QueueSubmitBatchInfo submitBatchInfo;
        QueueSubmitInfo submitInfo;
        submitBatchInfo.fenceToSignal = nullptr;
        submitBatchInfo.submitInfoCount = 1;
        submitBatchInfo.submitInfos = &submitInfo;
        submitInfo.commandBuffers = &updateCmd;
        submitInfo.commandCount = 1;

        _uploadQueue->submitCommandBuffers(submitBatchInfo);

        _uploadQueue->waitIdle();
        //
        res_descriptor_t res;
        m_uniformDescriptor.type = ArgumentDescriptorType::UniformBuffer;
        m_uniformdescriptor_binder.handle = ArgumentGroup::GetDescriptorHandle("Argument1", pipelineDesc );
        m_uniformDescriptor.bufferRange = 64;

        res.type = ArgumentDescriptorType::Sampler;
        res.sampler = m_samplerState;
        res.handle = ArgumentGroup::GetDescriptorHandle("triSampler", pipelineDesc );
        _argumentGroup->updateDescriptor(res);
        
        res.type = ArgumentDescriptorType::Image;
        res.texture = _texture;
        res.handle = ArgumentGroup::GetDescriptorHandle("triTexture", pipelineDesc );
        //
        _argumentGroup->updateDescriptor(res);
        //
        _flightIndex = 0;
        //
        return true;
    }

    void App::tick() {
        
        _device->waitForFence( _frameCompleteFences[_flightIndex] );
        _uniformAllocator->tick();
        uint32_t imageIndex = _swapchain->acquireNextImage( _device, _flightIndex );
        // _frameCompleteFences[_flightIndex]->
        IRenderPass* mainRenderPass = _swapchain->renderPass(imageIndex);
        
        auto cmdbuf = _commandBuffers[_flightIndex];

        cmdbuf->beginEncode(); {

            static uint64_t angle = 0;
            ++angle;

            glm::mat4 mmm = glm::rotate<float>( glm::mat4(), (float)angle, glm::vec3(0, 1, 0) );
            glm::mat4 vvv = glm::lookAt( glm::vec3(2, 2, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
            glm::mat4 ppp = glm::perspective<float>( 90, (float)_width/(float)_height, 0.1, 300);
            auto mvp = ppp * vvv * mmm;
            
            auto ubo = _uniformAllocator->allocate(sizeof(mvp));
            ubo.writeData(0, &mvp, sizeof(mvp));
            //
            m_uniformDescriptor.bufferOffset = ubo.offset();
            m_uniformDescriptor.buffer = ubo.buffer(); 
            _argumentGroup->updateDescriptor(m_uniformDescriptor);

            auto resourceEncoder = cmdbuf->resourceCommandEncoder();
            resourceEncoder->prepareArgumentGroup(_argumentGroup);
            resourceEncoder->endEncode();
            //
            RenderPassClearValues clearValues;
            clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
            clearValues.depth = 1.0f;
            clearValues.depth = 0xffffffff;

            mainRenderPass->setClearValues(clearValues);

            auto renderCommandEncoder = cmdbuf->renderCommandEncoder( mainRenderPass ); {
                renderCommandEncoder->setLineWidth(1.0f);
                renderCommandEncoder->setViewport(0, 0, _width, _height, 0, 1.0f );
                renderCommandEncoder->setScissor( 0, 0, _width, _height );
                renderCommandEncoder->bindPipeline(_pipeline);
                renderCommandEncoder->bindArgumentGroup(_argumentGroup);
                renderCommandEncoder->drawIndexed( m_drawable, 0, m_indexBuffer->size() / sizeof(uint16_t));
                // renderCommandEncoder->draw( m_drawable, 3, 0 );
            }
            renderCommandEncoder->endEncode();
        }
        cmdbuf->endEncode();

        QueueSubmitInfo submitInfo = {}; {
            Semaphore* imageAvailSemaphore = _swapchain->imageAvailSemaphore();
            submitInfo.commandBuffers = &cmdbuf;
            submitInfo.commandCount = 1;
            submitInfo.semaphoresToSignal = &_renderCompleteSemaphores[_flightIndex];
            submitInfo.semaphoresToSignalCount = 1;
            submitInfo.semaphoresToWait = &imageAvailSemaphore;
            submitInfo.semaphoresToWaitCount = 1;
        }

        QueueSubmitBatchInfo submitBatch;{
            submitBatch.fenceToSignal = _frameCompleteFences[_flightIndex];
            submitBatch.submitInfos = &submitInfo;
            submitBatch.submitInfoCount = 1;
        }
        bool submitRst = _graphicsQueue->submitCommandBuffers(submitBatch);
        assert(submitRst);
        _swapchain->present( _device, _graphicsQueue, _renderCompleteSemaphores[_flightIndex] );

    }
        
    void App::resize(uint32_t _width, uint32_t _height) {
        _swapchain->resize( _device, _width, _height );
        //
        _width = _width;
        _height = _height;
    }

    void App::release() {
    }

    const char * App::title() {
        return "Sphere";
    }
        
    uint32_t App::rendererType() {
        return 0;
    }

}

ugi::App theapp;

UGIApplication* GetApplication() {
    return &theapp;
}