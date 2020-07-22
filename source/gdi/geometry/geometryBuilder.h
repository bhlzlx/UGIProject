﻿
#pragma once
#include <hgl/math/Vector.h>
#include <algorithm>
#include <utility>
#include <vector>
#include <cstdint>

namespace ugi {

    namespace gdi {

        typedef uint32_t GeometryHandle;

        struct GeometryVertex;
        class GeometryDrawData;
        class GDIContext;
        class IGeometryBuilder {
        public:
            virtual void beginBuild() = 0;
            virtual GeometryDrawData* endBuild() = 0;
            virtual void drawLine( const hgl::Vector2f& pointStart, const hgl::Vector2f& pointEnd, float width, uint32_t color ) = 0;
            virtual GeometryHandle drawRect( float x, float y, float width, float height, uint32_t color, bool dynamic = false ) = 0;
            virtual GeometryHandle drawVertices( const GeometryVertex* vertices, uint32_t vertexCount, const uint16_t* indices, uint32_t indexCount ) = 0;
            ~IGeometryBuilder() {}
        };

        IGeometryBuilder* CreateGeometryBuilder( GDIContext* context );

    }

}

    