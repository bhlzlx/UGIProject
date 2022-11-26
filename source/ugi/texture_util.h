#pragma once

#include <ugi/ugi_declare.h>
#include <cstdint>

namespace ugi {
    
    /**
     * @brief Create a Texture KTX format
     * usage example:
     *      Texture* texture = CreateTextureKTX(device, (uint8_t const*)buffer, ktxfile->size(), _renderContext->asyncLoadManager(), 
                [this,i,device](void* res, CommandBuffer* cb) {
                    _textures[i] = (Texture*)res;
                    auto resEnc = cb->resourceCommandEncoder();
                    resEnc->imageTransitionBarrier(
                        _textures[i], ResourceAccessType::ShaderRead, 
                        PipelineStages::Bottom, StageAccess::Write,
                        PipelineStages::FragmentShading, StageAccess::Read,
                        nullptr
                    );
                    resEnc->endEncode();
                    _textures[i]->markAsUploaded();
                }
            );
     * 
     * @param device 
     * @param data 
     * @param dataLen 
     * @param asyncLoadMgr 
     * @param callback 
     * @return Texture* 
     */
    Texture* CreateTextureKTX(Device* device, uint8_t const* data, uint32_t dataLen, GPUAsyncLoadManager* asyncLoadMgr, AsyncLoadCallback&& callback);

    Texture* CreateTexturePNG(Device* device, uint8_t const* data, uint32_t dataLen, GPUAsyncLoadManager* asyncLoadMgr, AsyncLoadCallback&& callback);

    void GenerateMipmap(Device* deivce, Texture* texture);

}