#include "MemTest.h"
#include <random>
#include <chrono>
#include <iostream>
#include "bbs.h"

#include <cmath>

namespace ugi {

    bool MemTest::initialize( void* _wnd, hgl::assets::AssetsSource* assetsSource ) {
        std::random_device rd;
	//
	bbs::IDAllocator allocator;
	allocator.initialize(256*4096, 128);
	uint64_t requestSize = 0;
	uint64_t heapSize = 256*4096;
	size_t offset;
	uint16_t id;
	allocator.allocate(64, offset, id);
	//
	for( uint32_t i = 0; i<1; ++i) {
		std::vector<uint16_t> ids;
		ids.reserve(4096);
		// 计算时间间隔最好用steady_clock
		std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
		for( uint32_t j = 0; j<2000; ++j ) {
			uint32_t rand = rd();
			rand = rand%1024 + 64;
			size_t offset;
			uint16_t id;
			bool rst = allocator.allocate( rand, offset, id);
			if( rst ) {
				ids.push_back(id);
				requestSize += rand;
			}
		}	
		std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
		std::chrono::steady_clock::duration d = end - start;
		if (d == std::chrono::steady_clock::duration::zero())    //0时间长度的表示
			std::cout << "no time elapsed" << std::endl;
		std::cout << "time elapsed" << std::chrono::duration_cast<std::chrono::microseconds>(d).count() << "with count"<< ids.size() << std::endl;

		start = std::chrono::steady_clock::now();

		std::cout<<"percent : "<< (float)requestSize/(float)heapSize<< std::endl;

		for( auto id : ids) {
			allocator.free(id);
		}

		end = std::chrono::steady_clock::now();
		d = end - start;
		if (d == std::chrono::steady_clock::duration::zero())    //0时间长度的表示
			std::cout << "no time elapsed" << std::endl;
		std::cout << std::chrono::duration_cast<std::chrono::microseconds>(d).count() << std::endl;
#ifndef NDEBUG
		allocator.empty();
#endif
	}
        return true;
    }

    void MemTest::tick() {
    }
        
    void MemTest::resize(uint32_t _width, uint32_t _height) {
    }

    void MemTest::release() {
    }

    const char * MemTest::title() {
        return "MemTest";
    }
        
    uint32_t MemTest::rendererType() {
        return 0;
    }

}

ugi::MemTest theapp;

UGIApplication* GetApplication() {
    return &theapp;
}