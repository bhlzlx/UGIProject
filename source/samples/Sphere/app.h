#include <UGIApplication.h>
#include <cstdio>
#include <algorithm>
#include <ugi/UGIDeclare.h>
#include <ugi/UGITypes.h>
#include <ugi/Argument.h>
#include <hgl/math/Vector.h>
// #include <glm/glm.hpp>

namespace ugi {

    struct CVertex {
        hgl::vec3<float> position;
        hgl::vec3<float> normal;
        hgl::vec2<float> coord;
	};

	void CreateSphere(int _nstack, int _nslice, std::vector<CVertex>& _vertices, std::vector<uint16_t>& _indices) {
		assert(_nstack >= 3);
		assert(_nslice >= 3);
		//
		float dy = 2.0f / _nstack;
		int mid = (_nstack - 1) / 2;
		int midSliceCount = mid + _nslice;

		std::vector< CVertex > vertices;

		for (int stk = 0; stk < _nstack; ++stk) {
			int sliceCount = 0;
			if (_nstack & 0x1) { // 奇数
				sliceCount = midSliceCount - abs(mid - stk);
			}
			else {
				if (stk > mid) {
					sliceCount = midSliceCount - abs(mid - stk + 1);
				}
				else {
					sliceCount = midSliceCount - abs(mid - stk);
				}
			}
			float yarc = 3.141592654f / (_nstack - 1) * stk;
			float y = cos(yarc);
			float xzref = sin(yarc);
			float x = 0;
			float z = 0;
			float dxarc = 3.141592654f * 2.0f / (sliceCount - 1);
			for (int slc = 0; slc < sliceCount; ++slc) {
				float xarc = dxarc * slc;
				x = cos(xarc) * xzref;
				z = sin(xarc) * xzref;
				//
				hgl::vec3<float> pos(x, y, z);
				hgl::vec3<float> normal(x, y, z);
				hgl::vec2<float> coord(1.0f - 1.0f / (sliceCount - 1) * slc, 1.0f - (y + 1.0f) / 2.0f);
				//
				CVertex vertex;
				vertex.position = pos;
				vertex.normal = normal;
				vertex.coord = coord;
				//
				_vertices.push_back(vertex);
			}
			//printf("stack : %d , slice : %d", stk, sliceCount);
		}
		// create indices
		size_t count = _vertices.size();
		size_t counter = 0;
		if (_nstack & 0x1) { // 奇数
			for (int stk = 0; stk < mid; ++stk) {
				int sliceCount = midSliceCount - abs(mid - stk);
				for (size_t ct = 1; ct <= sliceCount; ++ct)
				{
					uint16_t sixple[6];
					sixple[0] = counter;
					sixple[1] = counter + sliceCount;
					sixple[2] = sixple[1] + 1;
					//
					sixple[3] = (count - sixple[0]) - 1;
					sixple[4] = (count - sixple[1]) - 1;
					sixple[5] = (count - sixple[2]) - 1;
					//
					for (auto index : sixple)
						_indices.push_back(index);
					if (stk != 0 && ct != sliceCount) {
						sixple[0] = counter;
						sixple[1] = counter + sliceCount + 1;
						sixple[2] = counter + 1;

						sixple[3] = (count - sixple[0]) - 1;
						sixple[4] = (count - sixple[1]) - 1;
						sixple[5] = (count - sixple[2]) - 1;


						//sixple[3] = count - sixple[0] - 1; sixple[4] = count - sixple[1] - 1; sixple[5] = sixple[4] - 1;
						for (auto index : sixple)
							_indices.push_back(index);
					}
					//
					++counter;
				}
			}
		}
		else { // 偶数
			for (int stk = 0; stk < mid; ++stk) {
				int sliceCount = 0;
				if (stk > mid) {
					sliceCount = midSliceCount - abs(mid - stk + 1);
				}
				else {
					sliceCount = midSliceCount - abs(mid - stk);
				}
				//
				for (size_t ct = 1; ct <= sliceCount; ++ct)
				{
					uint16_t sixple[6];
					sixple[0] = counter;
					sixple[1] = counter + sliceCount;
					sixple[2] = sixple[1] + 1;
					//
					sixple[3] = (count - sixple[0]) - 1;
					sixple[4] = (count - sixple[1]) - 1;
					sixple[5] = (count - sixple[2]) - 1;
					//
					for (auto index : sixple)
						_indices.push_back(index);
					if (stk != 0 && ct != sliceCount) {
						sixple[0] = counter;
						sixple[1] = counter + sliceCount + 1;
						sixple[2] = counter + 1;

						sixple[3] = (count - sixple[0]) - 1;
						sixple[4] = (count - sixple[1]) - 1;
						sixple[5] = (count - sixple[2]) - 1;


						//sixple[3] = count - sixple[0] - 1; sixple[4] = count - sixple[1] - 1; sixple[5] = sixple[4] - 1;
						for (auto index : sixple)
							_indices.push_back(index);
					}
					//
					++counter;
				}
			}
			//
			for (int ct = 0; ct < midSliceCount - 1; ++ct) {
				uint16_t sixple[6];
				sixple[0] = counter;
				sixple[1] = sixple[0] + midSliceCount;
				sixple[2] = sixple[0] + 1;
				//
				sixple[3] = (count - sixple[0]) - 1;
				sixple[4] = (count - sixple[1]) - 1;
				sixple[5] = (count - sixple[2]) - 1;
				for (auto index : sixple)
					_indices.push_back(index);
				++counter;
			}
		}
	}

    class App : public UGIApplication {
    private:
        void*                   _hwnd;                                             //
        ugi::RenderSystem*      _renderSystem;                                     //
        ugi::Device*            _device;                                           //
        ugi::Swapchain*         _swapchain;                                        //
        //
        ugi::Fence*             _frameCompleteFences[MaxFlightCount];              // command buffer 被GPU消化完会给 fence 一个 signal, 用于双缓冲或者多缓冲逻辑隔帧等待
        ugi::Semaphore*         _renderCompleteSemaphores[MaxFlightCount];         // 用于GPU内部等待
        ugi::CommandBuffer*     _commandBuffers[MaxFlightCount];                   // command buffer for each frame
        ugi::CommandQueue*      _graphicsQueue;                                    // graphics queue
        ugi::CommandQueue*      _uploadQueue;                                      // upload queue
        ugi::Pipeline*          _pipeline;
        ///> ===========================================================================
        ugi::ArgumentGroup*     _argumentGroup;                                    // 
        // ugi::Buffer*            m_uniformBuffer;
        ugi::Texture*           m_texture;
        ugi::SamplerState       m_samplerState;                                     //
        ugi::Buffer*            m_vertexBuffer;                                     //
        ugi::Buffer*            m_indexBuffer;
        ugi::Drawable*          m_drawable;

        ugi::UniformAllocator*  _uniformAllocator;
        ResourceDescriptor      m_uniformDescriptor;
        //
        uint32_t                _flightIndex;                                      // flight index
        //
        float                   _width;
        float                   _height;
    public:
        virtual bool initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource );
        virtual void resize( uint32_t _width, uint32_t _height );
        virtual void release();
        virtual void tick();
        virtual const char * title();
        virtual uint32_t rendererType();
    };

}

UGIApplication* GetApplication();