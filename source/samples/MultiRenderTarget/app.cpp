#include "app.h"
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

        m_renderSystem = new ugi::RenderSystem();

        ugi::DeviceDescriptor descriptor; {
            descriptor.apiType = ugi::GRAPHICS_API_TYPE::VULKAN;
            descriptor.deviceType = ugi::GRAPHICS_DEVICE_TYPE::DISCRETE;
            descriptor.debugLayer = 1;
            descriptor.graphicsQueueCount = 1;
            descriptor.transferQueueCount = 1;
            descriptor.wnd = _wnd;
        }
        m_device = m_renderSystem->createDevice(descriptor, _archive);
        m_uniformAllocator = m_device->createUniformAllocator();
        m_swapchain = m_device->createSwapchain( _wnd );
        //
        // command queues
        m_graphicsQueue = m_device->graphicsQueues()[0];
        m_uploadQueue = m_device->transferQueues()[0];
        //
        for( size_t i = 0; i<MaxFlightCount; ++i) {
            m_frameCompleteFences[i] = m_device->createFence();
            m_renderCompleteSemaphores[i] = m_device->createSemaphore();
            m_commandBuffers[i] = m_graphicsQueue->createCommandBuffer( m_device );
        }
        pipelineDesc.renderState.cullMode = CullMode::None;
        pipelineDesc.renderState.blendState.enable = false;
        m_pipeline = m_device->createPipeline(pipelineDesc);
        //
        m_argumentGroup = m_pipeline->createArgumentGroup();

        std::vector<CVertex>        vertices;
        std::vector<uint16_t>       indices;
        CreateSphere(8, 12, vertices, indices);

        m_vertexBuffer = m_device->createBuffer( BufferType::VertexBuffer, sizeof(CVertex) * vertices.size() );
        m_indexBuffer = m_device->createBuffer( BufferType::IndexBuffer, sizeof(uint16_t) * indices.size() );
        Buffer* vertexStagingBuffer = m_device->createBuffer( BufferType::StagingBuffer, m_vertexBuffer->size() );
        Buffer* indexStagingBuffer = m_device->createBuffer( BufferType::StagingBuffer, m_indexBuffer->size() );

        void* ptr = vertexStagingBuffer->map( m_device );
        memcpy(ptr, vertices.data(), m_vertexBuffer->size());
        vertexStagingBuffer->unmap(m_device);

        ptr = indexStagingBuffer->map( m_device );
        memcpy(ptr, indices.data(), m_indexBuffer->size());
        indexStagingBuffer->unmap(m_device);

        auto updateCmd = m_uploadQueue->createCommandBuffer( m_device );

        updateCmd->beginEncode();

        auto resourceEncoder = updateCmd->resourceCommandEncoder();

        BufferSubResource subRes;
        subRes.offset = 0; subRes.size = m_vertexBuffer->size();
        resourceEncoder->updateBuffer( m_vertexBuffer, vertexStagingBuffer, &subRes, &subRes );
        subRes.size = m_indexBuffer->size();
        resourceEncoder->updateBuffer( m_indexBuffer, indexStagingBuffer, &subRes, &subRes );

        m_drawable = m_device->createDrawable(pipelineDesc);
        m_drawable->setVertexBuffer( m_vertexBuffer, 0, 0 );
        m_drawable->setIndexBuffer( m_indexBuffer, 0 );
        //
        TextureDescription texDesc;
        texDesc.format = UGIFormat::RGBA8888_UNORM;
        texDesc.depth = 1;
        texDesc.width = 16;
        texDesc.height = 16;
        texDesc.type = TextureType::Texture2D;
        texDesc.mipmapLevel = 1;
        texDesc.arrayLayers = 1;
        m_texture = m_device->createTexture(texDesc, ResourceAccessType::ShaderRead );

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
        auto texStagingBuffer =  m_device->createBuffer( BufferType::StagingBuffer, sizeof(texData) );
        ptr = texStagingBuffer->map(m_device);
        memcpy(ptr, texData, sizeof(texData));
        texStagingBuffer->unmap(m_device);

        TextureSubResource texSubRes;
        texSubRes.baseLayer = 0;
        texSubRes.offset = { 0, 0, 0 };
        texSubRes.size.depth = 1;
        texSubRes.size.width = texSubRes.size.height = 8;
        texSubRes.mipLevelCount = 1;
        texSubRes.baseMipLevel = 0;
        texSubRes.layerCount = 1;
        subRes.size = sizeof(texData);

        resourceEncoder->updateImage( m_texture, texStagingBuffer, &texSubRes, &subRes );
        texSubRes.offset.x = 8;
        resourceEncoder->updateImage( m_texture, texStagingBuffer, &texSubRes, &subRes );
        texSubRes.offset.y = 8; texSubRes.offset.x = 0;
        resourceEncoder->updateImage( m_texture, texStagingBuffer, &texSubRes, &subRes );
        texSubRes.offset.y = 8; texSubRes.offset.x = 8;
        resourceEncoder->updateImage( m_texture, texStagingBuffer, &texSubRes, &subRes );

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

        m_uploadQueue->submitCommandBuffers(submitBatchInfo);

        m_uploadQueue->waitIdle();
        //
        ResourceDescriptor res;
        m_uniformDescriptor.type = ArgumentDescriptorType::UniformBuffer;
        m_uniformDescriptor.descriptorHandle = ArgumentGroup::GetDescriptorHandle("Argument1", pipelineDesc );
        m_uniformDescriptor.bufferRange = 64;

        res.type = ArgumentDescriptorType::Sampler;
        res.sampler = m_samplerState;
        res.descriptorHandle = ArgumentGroup::GetDescriptorHandle("triSampler", pipelineDesc );
        m_argumentGroup->updateDescriptor(res);
        
        res.type = ArgumentDescriptorType::Image;
        res.texture = m_texture;
        res.descriptorHandle = ArgumentGroup::GetDescriptorHandle("triTexture", pipelineDesc );
        //
        m_argumentGroup->updateDescriptor(res);
        //
        m_flightIndex = 0;
        //
        return true;
    }

    void App::tick() {
        
        m_device->waitForFence( m_frameCompleteFences[m_flightIndex] );
        m_uniformAllocator->tick();
        uint32_t imageIndex = m_swapchain->acquireNextImage( m_device, m_flightIndex );
        // m_frameCompleteFences[m_flightIndex]->
        IRenderPass* mainRenderPass = m_swapchain->renderPass(imageIndex);
        
        auto cmdbuf = m_commandBuffers[m_flightIndex];

        cmdbuf->beginEncode(); {

            static uint64_t angle = 0;
            ++angle;

            glm::mat4 mmm = glm::rotate<float>( glm::mat4(), (float)angle, glm::vec3(0, 1, 0) );
            glm::mat4 vvv = glm::lookAt( glm::vec3(2, 2, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
            glm::mat4 ppp = glm::perspective<float>( 90, (float)m_width/(float)m_height, 0.1, 300);
            auto mvp = ppp * vvv * mmm;
            
            auto ubo = m_uniformAllocator->allocate(sizeof(mvp));
            ubo.writeData(0, &mvp, sizeof(mvp));
            //
            m_uniformDescriptor.bufferOffset = ubo.offset();
            m_uniformDescriptor.buffer = ubo.buffer(); 
            m_argumentGroup->updateDescriptor(m_uniformDescriptor);

            auto resourceEncoder = cmdbuf->resourceCommandEncoder();
            resourceEncoder->prepareArgumentGroup(m_argumentGroup);
            resourceEncoder->endEncode();
            //
            RenderPassClearValues clearValues;
            clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
            clearValues.depth = 1.0f;
            clearValues.depth = 0xffffffff;

            mainRenderPass->setClearValues(clearValues);

            auto renderCommandEncoder = cmdbuf->renderCommandEncoder( mainRenderPass ); {
                renderCommandEncoder->setLineWidth(1.0f);
                renderCommandEncoder->setViewport(0, 0, m_width, m_height, 0, 1.0f );
                renderCommandEncoder->setScissor( 0, 0, m_width, m_height );
                renderCommandEncoder->bindPipeline(m_pipeline);
                renderCommandEncoder->bindArgumentGroup(m_argumentGroup);
                renderCommandEncoder->drawIndexed( m_drawable, 0, m_indexBuffer->size() / sizeof(uint16_t));
                // renderCommandEncoder->draw( m_drawable, 3, 0 );
            }
            renderCommandEncoder->endEncode();
        }
        cmdbuf->endEncode();

        QueueSubmitInfo submitInfo = {}; {
            Semaphore* imageAvailSemaphore = m_swapchain->imageAvailSemaphore();
            submitInfo.commandBuffers = &cmdbuf;
            submitInfo.commandCount = 1;
            submitInfo.semaphoresToSignal = &m_renderCompleteSemaphores[m_flightIndex];
            submitInfo.semaphoresToSignalCount = 1;
            submitInfo.semaphoresToWait = &imageAvailSemaphore;
            submitInfo.semaphoresToWaitCount = 1;
        }

        QueueSubmitBatchInfo submitBatch;{
            submitBatch.fenceToSignal = m_frameCompleteFences[m_flightIndex];
            submitBatch.submitInfos = &submitInfo;
            submitBatch.submitInfoCount = 1;
        }
        bool submitRst = m_graphicsQueue->submitCommandBuffers(submitBatch);
        assert(submitRst);
        m_swapchain->present( m_device, m_graphicsQueue, m_renderCompleteSemaphores[m_flightIndex] );

    }
        
    void App::resize(uint32_t _width, uint32_t _height) {
        m_swapchain->resize( m_device, _width, _height );
        //
        m_width = _width;
        m_height = _height;
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