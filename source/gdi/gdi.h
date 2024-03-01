#pragma once
#include <algorithm>
#include <utility>
#include <ugi/ugi_declare.h>
#include <ugi/ugi_types.h>
#include <glm/glm.hpp>

namespace ugi {

    namespace gdi {

        // 存储 GDI 设备上下文的状态
        // 先 使用GeometryBuilder 在CPU端组装顶点以及索引数据
        class GDIContext {
        private:
            ugi::Device*                _device;
            hgl::assets::AssetsSource*  _assetsSource;
            ugi::pipeline_desc_t        _pipelineDesc;
            ugi::GraphicsPipeline*      _pipeline;
            //
            glm::vec2                   _standardSize;
            glm::vec2                   _size;          // context size( width,height )
            glm::vec2                   _scale;         // 
            float                       _commonScale;
            //
            uint32_t                    _uniformMaxSize;
        public:
            GDIContext( ugi::Device* device, hgl::assets::AssetsSource* assetsSource )
                : _device( device )
                , _assetsSource( assetsSource )
                , _pipeline(nullptr)
                , _standardSize(  600, 480 )
            {
            }

            bool initialize();

            hgl::assets::AssetsSource* assetsSource() const {
                return _assetsSource;
            }

            ugi::GraphicsPipeline* pipeline() const ;
            const ugi::pipeline_desc_t& pipelineDescription() const ;
            //
            void setSize(const hgl::Vector2f& size);
            //
            ugi::Device* device() { return _device;  };
            const hgl::Vector2f& size() { return _size; }
            
        };

    }
}