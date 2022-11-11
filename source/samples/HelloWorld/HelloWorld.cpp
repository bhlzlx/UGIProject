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
#include <ugi/UniformBuffer.h>
#include <ugi/Descriptor.h>
#include <ugi/MeshBufferAllocator.h>
#include <ugi/render_components/MeshPrimitive.h>
#include <ugi/async_load/AsyncLoadManager.h>
#include <ugi/render_components/Renderable.h>
#include <ugi/render_components/PipelineMaterial.h>
// #include <hgl/assets/AssetsSource.h>

#include <cmath>

namespace ugi {


    bool Render::initialize() {
        auto material = _pipeline->createMaterial({"Argument1", "triSampler", "triTexture"}, {});
        _uboptor = material->descriptors()[0];
        _samptor = material->descriptors()[1];
        _texptor = material->descriptors()[2];
        _bufferAllocator = new MeshBufferAllocator();
        auto rst = _bufferAllocator->initialize(_device, 4096);
        return rst;
    }

    void Render::setRasterization(raster_state_t rasterState) {
        _pipeline->setRasterizationState(rasterState);
    }

    void Render::tick() {
        _pipeline->resetMaterials();
    }

    void Render::bind(RenderCommandEncoder* encoder) {
        encoder->bindPipeline(_pipeline);
    }

    void Render::draw(RenderCommandEncoder* enc, Renderable* renderable) {
        _pipeline->applyMaterial(renderable->material());
        _pipeline->flushMaterials(enc->commandBuffer());
        enc->draw(renderable->mesh(), renderable->mesh()->indexCount());
    }

    void Render::setUBO(Renderable* renderable, uint8_t* data) {
        auto mtl = renderable->material();
        auto ubo = _uniformAllocator->allocate(_uboptor.res.buffer.size);
        ubo.writeData(0, data, _uboptor.res.buffer.size);
        _uboptor.res.buffer.buffer = (size_t)ubo.buffer()->buffer();
        _uboptor.res.buffer.offset = ubo.offset();
        mtl->updateDescriptor(_uboptor);
    }

    void Render::setSampler(Renderable* renderable, sampler_state_t sampler) {
        auto mtl = renderable->material();
        _samptor.res.samplerState = sampler;
        mtl->updateDescriptor(_samptor);
    }

    void Render::setTexture(Renderable* renderable, image_view_t imageView) {
        auto mtl = renderable->material();
        _texptor.res.imageView = imageView.handle;
        mtl->updateDescriptor(_texptor);
    }

    Renderable* Render::createRenderable(uint8_t const* vd, uint32_t vdsize, uint16_t const* id, uint32_t indexCount, GPUAsyncLoadManager* asyncLoadManager) {
        auto mesh = Mesh::CreateMesh(
            _device, _bufferAllocator, asyncLoadManager,
            (uint8_t const*)vd, vdsize,
            id, indexCount,
            _pipeline->desc().vertexLayout,
            _pipeline->desc().topologyMode,
            ugi::polygon_mode_t::Fill,
            [](CommandBuffer* cb){}
        );
        auto material = _pipeline->createMaterial({"Argument1", "triSampler", "triTexture"}, {});
        auto renderable = new Renderable(mesh, material, _pipeline, raster_state_t());
        return renderable;
    }

    bool RenderContext::initialize(void* wnd, ugi::DeviceDescriptor deviceDesc, comm::IArchive* archive) {
        renderSystem = new RenderSystem();
        device = renderSystem->createDevice(deviceDesc, archive);
        uniformAllocator = device->createUniformAllocator();
        descriptorSetAllocator = device->descriptorSetAllocator();
        swapchain = device->createSwapchain(wnd);
        graphicsQueue = device->graphicsQueues()[0];
        uploadQueue = device->transferQueues()[0];
        asyncLoadManager = new ugi::GPUAsyncLoadManager();
        for( size_t i = 0; i<MaxFlightCount; ++i) {
            frameCompleteFences[i] = device->createFence();
            renderCompleteSemaphores[i] = device->createSemaphore();
            commandBuffers[i] = graphicsQueue->createCommandBuffer(device);
        }
        return true;
    }

    bool HelloWorld::initialize( void* _wnd, comm::IArchive* arch) {
        auto pipelineFile = arch->openIStream("/shaders/triangle/pipeline.bin", {comm::ReadFlag::binary});
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
        auto pipeline = _renderContext.device->createGraphicsPipeline(pipelineDesc);
        auto bufferAllocator = new MeshBufferAllocator();
        bufferAllocator->initialize(_renderContext.device, 1024);

        _render = new Render(_renderContext.device, pipeline, bufferAllocator, _renderContext.uniformAllocator);
        _render->initialize();

        float vertexData[] = {
            -0.5, -0.5, 0, 0, 0,
            0, 0.5, 0, 0.5, 1.0,
            0.5, -0.5, 0, 1.0, 0
        };
        uint16_t indexData[] = {
            0, 1, 2
        };
        _renderable = _render->createRenderable((uint8_t const*)vertexData, sizeof(vertexData), indexData, 3, _renderContext.asyncLoadManager);
        tex_desc_t texDesc;
        texDesc.format = UGIFormat::RGBA8888_UNORM;
        texDesc.depth = 1;
        texDesc.width = 16;
        texDesc.height = 16;
        texDesc.type = TextureType::Texture2D;
        texDesc.mipmapLevel = 1;
        texDesc.arrayLayers = 1;
        _texture = _renderContext.device->createTexture(texDesc, ResourceAccessType::ShaderRead );
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
        std::vector<ImageRegion> regions;
        std::vector<uint32_t> offsets = {0, 0, 0, 0};
        ImageRegion region;
        region.offset = {};
        region.mipLevel = 0;
        region.arrayIndex = 0;
        region.arrayCount = 1;
        region.extent.height = region.extent.width = 8;
        region.extent.depth = 1;
        regions.push_back(region);
        region.offset.x = 8;
        regions.push_back(region);
        region.offset.x = 0; region.offset.y = 8;
        regions.push_back(region);
        region.offset.x = 8; region.offset.y = 8;
        regions.push_back(region);
        _texture->updateRegions(
            _renderContext.device, 
            regions.data(), regions.size(), 
            (uint8_t const*)texData, sizeof(texData), offsets.data(), 
            _renderContext.asyncLoadManager,
            [this](CommandBuffer* cb) {
                auto resEnc = cb->resourceCommandEncoder();
                resEnc->imageTransitionBarrier(
                    _texture, ResourceAccessType::ShaderRead, 
                    PipelineStages::FragmentShading, StageAccess::Read,
                    PipelineStages::FragmentShading, StageAccess::Read,
                    nullptr
                );
                resEnc->endEncode();
                _texture->markAsUploaded();
            }
        );
        image_view_param_t ivp;
        _imageView = _texture->createImageView(_renderContext.device, ivp);
        _render->setSampler(_renderable, _samplerState);
        _render->setTexture(_renderable, _imageView);
        _flightIndex = 0;
        return true;
    }

    void HelloWorld::tick() {
        _renderContext.device->waitForFence( _renderContext.frameCompleteFences[_flightIndex] );
        _renderContext.descriptorSetAllocator->tick();
        _renderContext.uniformAllocator->tick();
        _render->tick();
        uint32_t imageIndex = _renderContext.swapchain->acquireNextImage(_renderContext.device, _flightIndex);
        //
        IRenderPass* mainRenderPass = _renderContext.swapchain->renderPass(imageIndex);
        auto cmdbuf = _renderContext.commandBuffers[_flightIndex];
        cmdbuf->beginEncode(); {
            _renderContext.asyncLoadManager->tick(cmdbuf);
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
            _render->setUBO(_renderable, (uint8_t*)col2);
            //
            renderpass_clearval_t clearValues;
            clearValues.colors[0] = { 0.5f, 0.5f, 0.5f, 1.0f }; // RGBA
            clearValues.depth = 1.0f;
            clearValues.stencil = 0xffffffff;

            mainRenderPass->setClearValues(clearValues);

            auto renderEnc = cmdbuf->renderCommandEncoder(mainRenderPass); {
                if(_texture->uploaded() && _renderable->mesh()->prepared()) {
                    renderEnc->setLineWidth(1.0f);
                    renderEnc->setViewport(0, 0, _width, _height, 0, 1.0f );
                    renderEnc->setScissor( 0, 0, _width, _height );
                    _render->setRasterization(rasterizationState);
                    _render->bind(renderEnc);
                    _render->draw(renderEnc, _renderable);
                }
            }
            renderEnc->endEncode();
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