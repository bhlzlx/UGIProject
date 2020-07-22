﻿#pragma once
#include <hgl/math/Vector.h>
#include <algorithm>
#include <utility>
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>

namespace hgl {
    namespace assets {
        class AssetsSource;
    }
}

namespace ugi {

    namespace gdi {

        // 存储 GDI 设备上下文的状态
        // 先 使用GeometryBuilder 在CPU端组装顶点以及索引数据
        class GDIContext {
        private:
            ugi::Device*                _device;
            hgl::assets::AssetsSource*  _assetsSource;
            ugi::PipelineDescription    _pipelineDesc;
            ugi::Pipeline*              _pipeline;
            //  
            hgl::Vector2f               _size;          // context size( width,height )
            //
            uint32_t                    _uniformMaxSize;
        public:
            GDIContext( ugi::Device* device, hgl::assets::AssetsSource* assetsSource )
                : _device( device )
                , _assetsSource( assetsSource )
                , _pipeline(nullptr)
            {
            }

            bool initialize();

            ugi::Pipeline* pipeline() const noexcept;
            const ugi::PipelineDescription& pipelineDescription() const noexcept;
            //
            void setSize(const hgl::Vector2f& size);
            //
            ugi::Device* device() { return _device;  };
            const hgl::Vector2f& size() { return _size; }
        };

    }
}