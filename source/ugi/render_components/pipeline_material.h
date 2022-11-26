#pragma once
#include <ugi_types.h>
#include <unordered_map>
#include <vector>

namespace ugi {

    class Material {
        friend class GraphicsPipeline;
        friend class ComputePipeline;
    protected:
        std::vector<res_descriptor_t> descriptors_;
        //
        Material()
            : descriptors_ {}
        {}
    public:
        /**
         * @brief 
         * 更新材质参数
         * @param descriptor
         */
        void updateDescriptor(res_descriptor_t const& descriptor);

        /**
         * @brief 获取 descriptor 数据 (一般用不到)
         * 
         * @return std::vector<res_descriptor_t> const& 
         */
        std::vector<res_descriptor_t> const& descriptors() const {
            return descriptors_;
        }
    };

}