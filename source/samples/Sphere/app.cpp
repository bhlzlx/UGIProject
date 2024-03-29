﻿#include "app.h"
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
#include <hgl/assets/AssetsSource.h>
#include <hgl/math/Matrix.h>
#include <hgl/math/Vector.h>
#include <hgl/math/Math.h>
//#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace ugi {

    hgl::Matrix4f Perspective( float fov, float aspect, float zNear, float zFar) {
        assert(abs(aspect - std::numeric_limits<float>::epsilon()) > static_cast<float>(0));
		float const tanHalfFovy = tan( fov / static_cast<float>(2));
		hgl::Matrix4f mat = hgl::Matrix4f::identity;
		mat[0][0] = 1.0f / (aspect * tanHalfFovy);
		mat[1][1] = 1.0f / (tanHalfFovy);
		mat[3][2] = -1.0f;
        mat[2][2] = zFar / (zNear - zFar);
        mat[2][3] = -(zFar * zNear) / (zFar - zNear);
		return mat;
    }

    hgl::Matrix4f LookAt(
        hgl::Vector3f eye,
        hgl::Vector3f target,
        // === 逻辑坐标系的 Top 是Z ===
        hgl::Vector3f worldTop = hgl::Vector3f(0, 0, 1)
    ) 
    {
        hgl::Matrix4f Result = hgl::Matrix4f::identity;
        hgl::Vector3f sightDir = target - eye;
        sightDir.Normalize();
        hgl::Vector3f sightRight = sightDir.Cross(worldTop);
        sightRight.Normalize();
        hgl::Vector3f sightUp = sightRight.Cross(sightDir);
        sightUp.Normalize();
        hgl::Matrix3f viewRotateMat;
        viewRotateMat.SetRow(0, sightRight);
        viewRotateMat.SetRow(1, sightUp);
        viewRotateMat.SetRow(2, sightDir);
        //  计算Eye在View局部坐标系里的位置，即偏移
        Result.SetTranslatePart(
            hgl::Vector3f( eye.Dot(sightRight), eye.Dot(sightUp), eye.Dot(sightDir) )
        );
        Result.Set3x3Part(viewRotateMat);
        Result[3][0] = Result[3][1] = Result[3][2] = 0.0f;
        return  Result;
    }
}

namespace ugi {

    ugi::Buffer* planeVtxBuffer = nullptr;
    ugi::Buffer* planeIdxBuffer = nullptr;
    ugi::Drawable* planeDrawable = nullptr;

    bool App::initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource) {

        hgl::io::InputStream* pipelineFile = assetsSource->Open( hgl::UTF8String("/shaders/sphere/pipeline.bin"));
        auto pipelineFileSize = pipelineFile->GetSize();
        
        char* pipelineBuffer = (char*)malloc(pipelineFileSize);
        pipelineFile->ReadFully(pipelineBuffer,pipelineFileSize);
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

        // 因为我们buffer放同一块内存了，这里特殊处理一下
        uint32_t fullStride = 0;
        for( uint32_t i = 0; i< pipelineDesc.vertexLayout.bufferCount; ++i) {
            pipelineDesc.vertexLayout.buffers[i].offset = fullStride;
            fullStride+=pipelineDesc.vertexLayout.buffers[i].stride;
        }
        for( uint32_t i = 0; i< pipelineDesc.vertexLayout.bufferCount; ++i) {
            pipelineDesc.vertexLayout.buffers[i].stride = fullStride;
        }
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
        _device = _renderSystem->createDevice(descriptor, assetsSource);
        _uniformAllocator = _device->createUniformAllocator();
        _swapchain = _device->createSwapchain( _wnd );
        // command queues
        _graphicsQueue = _device->graphicsQueues()[0];
        _uploadQueue = _device->transferQueues()[0];
        //
        for( size_t i = 0; i<MaxFlightCount; ++i) {
            _frameCompleteFences[i] = _device->createFence();
            _renderCompleteSemaphores[i] = _device->createSemaphore();
            _commandBuffers[i] = _graphicsQueue->createCommandBuffer( _device );
        }
        pipelineDesc.renderState.cullMode = cull_mode_t::None;
        pipelineDesc.renderState.blendState.enable = false;
        _pipeline = _device->createGraphicsPipeline(pipelineDesc);
        //
        _argumentGroup = _pipeline->createArgumentGroup();

        std::vector<CVertex>        vertices;
        std::vector<uint16_t>       indices;
        CreateSphere(16, 16, vertices, indices);

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
        m_drawable->setVertexBuffer( m_vertexBuffer, 1, 12 );
        m_drawable->setVertexBuffer( m_vertexBuffer, 2, 24 );
        m_drawable->setIndexBuffer( m_indexBuffer, 0 );

        planeDrawable = _device->createDrawable(pipelineDesc);
        planeDrawable->setVertexBuffer( planeVtxBuffer, 0, 0 );
        planeDrawable->setVertexBuffer( planeVtxBuffer, 1, 12 );
        planeDrawable->setVertexBuffer( planeVtxBuffer, 2, 24 );
        planeDrawable->setIndexBuffer( planeIdxBuffer, 0 );

        //
        tex_desc_t texDesc;
        texDesc.format = UGIFormat::RGBA8888_UNORM;
        texDesc.depth = 1;
        texDesc.width = 16;
        texDesc.height = 16;
        texDesc.type = TextureType::Texture2D;
        texDesc.mipmapLevel = 1;
        texDesc.layerCount = 1;
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

        image_region_t region;
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
        //
		Semaphore* imageAvailSemaphore = _swapchain->imageAvailSemaphore();
		QueueSubmitInfo submitInfo {
			&updateCmd,
			1,
			nullptr,// submitInfo.semaphoresToWait
			0,
			nullptr, // submitInfo.semaphoresToSignal
			0
		};
		QueueSubmitBatchInfo submitBatch(&submitInfo, 1, _frameCompleteFences[_flightIndex]);

        _uploadQueue->submitCommandBuffers(submitBatch);

        _uploadQueue->waitIdle();
        //
        res_descriptor_t res;
        m_uniformDescriptor.type = res_descriptor_type::UniformBuffer;
        m_uniformdescriptor_binder.handle = DescriptorBinder::GetDescriptorHandle("Argument", pipelineDesc );
        m_uniformDescriptor.bufferRange = 64 * 3;

        res.type = res_descriptor_type::Sampler;
        res.sampler = m_samplerState;
        res.handle = DescriptorBinder::GetDescriptorHandle("triSampler", pipelineDesc );
        _argumentGroup->updateDescriptor(res);
        
        res.type = res_descriptor_type::Image;

        image_view_param_t ivp;
        ivp.red = ChannelMapping::zero;
        res.imageView = _texture->view(_device, ivp);
        res.handle = DescriptorBinder::GetDescriptorHandle("triTexture", pipelineDesc );
        //
        _argumentGroup->updateDescriptor(res);
        //
        _flightIndex = 0;
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

            hgl::Matrix4f mvp[3];
            mvp[0] = hgl::Matrix4f::RotateAxisAngle( hgl::Vector3f(0,0,1), (float)angle/180*3.1415926f );
            // mvp[0] = hgl::Matrix4f::Translate(hgl::Vector3f(4,4,4)) * mvp[0];
            mvp[1] = ugi::LookAt(
                hgl::Vector3f(5, 5, 5),         ///> eye
                hgl::Vector3f(0, 0, 0)          ///> target
            );
            mvp[2] = Perspective( 3.1415926f/4, (float)_width/(float)_height, 0.1f, 50.0f); //hgl::Matrix4f::OpenGLOrthoProjRH(0.1f, 50, _width, _height);
            _uniformAllocator->allocateForDescriptor( m_uniformDescriptor, mvp );
            _argumentGroup->updateDescriptor(m_uniformDescriptor);

            auto resourceEncoder = cmdbuf->resourceCommandEncoder();
            resourceEncoder->prepareArgumentGroup(_argumentGroup);
            resourceEncoder->endEncode();
            //
            renderpass_clearval_t clearValues;
            clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
            clearValues.depth = 1.0f;
            clearValues.stencil = 0xffffffff;

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

        Semaphore* imageAvailSemaphore = _swapchain->imageAvailSemaphore();
		QueueSubmitInfo submitInfo {
			&cmdbuf,
			1,
			&imageAvailSemaphore,// submitInfo.semaphoresToWait
			1,
			&_renderCompleteSemaphores[_flightIndex], // submitInfo.semaphoresToSignal
			1
		};

        QueueSubmitBatchInfo submitBatch( &submitInfo, 1, _frameCompleteFences[_flightIndex]);

        bool submitRst = _graphicsQueue->submitCommandBuffers(submitBatch);
        assert(submitRst);
        _swapchain->present( _device, _graphicsQueue, _renderCompleteSemaphores[_flightIndex] );

    }
        
    void App::resize(uint32_t width, uint32_t height) {
        _width = width; _height = height;
        _swapchain->resize( _device, _width, _height );
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