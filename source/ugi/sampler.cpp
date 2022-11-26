#include "Sampler.h"

namespace ugi {

    class SamplerStateHashMethod {
    private:
    public:
        uint64_t operator() ( const sampler_state_t& state ) {
            UGIHash<APHash> hasher;
            hasher.hashPOD(state);
            return hasher;
        }
    };

    class SamplerCreateMethod {
    private:
    public:
        VkSampler operator()( Device* device, const sampler_state_t& samplerState ) {
            VkSamplerCreateInfo info; {
                info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
                info.flags = 0;
                info.pNext = nullptr;
                info.addressModeU = samplerAddressModeToVk(samplerState.u);
                info.addressModeV = samplerAddressModeToVk(samplerState.v);
                info.addressModeW = samplerAddressModeToVk(samplerState.w);
                info.compareOp = compareOpToVk(samplerState.compareFunction);
                info.compareEnable = samplerState.compareMode != TextureCompareMode::RefNone;
                info.magFilter = filterToVk(samplerState.mag);
                info.minFilter = filterToVk(samplerState.min);
                info.mipmapMode = mipmapFilterToVk(samplerState.mip);
                info.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
                info.anisotropyEnable = VK_FALSE;
                info.mipLodBias = 0;
                info.maxAnisotropy = 0;
                info.minLod = 0;
                info.maxLod = 0;
                info.unnormalizedCoordinates = 0;
            }
            VkSampler sampler;
            vkCreateSampler( device->device(), &info, nullptr, &sampler);
            return sampler;
        }
    };

    class SamplerDestroyMethod {
    private:
    public:
        void operator()( Device* device, VkSampler sampler) {
            vkDestroySampler(device->device(), sampler, nullptr);
        }
    };

    using SamplerPool = HashObjectPool< sampler_state_t, VkSampler, Device*, SamplerStateHashMethod, SamplerCreateMethod, SamplerDestroyMethod>;

    VkSampler CreateSampler( Device* device, const sampler_state_t& samplerState ) {
        uint64_t hashVal = 0;
        VkSampler sampler = SamplerPool::GetInstance()->getObject( samplerState, device, hashVal );
        return sampler;
    }

}