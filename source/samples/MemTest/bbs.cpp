#include "bbs.h"
#include <cassert>

namespace bbs {

	void IDAllocator::node_t::subNodeMarkAsExist( node_type_t _node ) {
		nodeBits|= 1<<(uint8_t)_node;
	}

	void IDAllocator::node_t::subNodeMarkAsNoneExist( node_type_t _node ) {
		nodeBits&= ~(1<<(uint8_t)_node);
	}

	bool IDAllocator::node_t::subNodeExist( node_type_t _node ) {
		return nodeBits&(1<<(uint8_t)_node);
	}
	bool IDAllocator::node_t::subNodeDoesNotExistAtAll() {
		return nodeBits == 0;
	}
	bool IDAllocator::node_t::subNodeAvailable( node_type_t _node ) { 	///> 某个节点是否有空闲空间来继续分配
		if(nodeAvailBits&(1<<(uint8_t)_node)) {
			return true;
		}
		return false;
	}
#ifndef NDEBUG
	void IDAllocator::empty() {
		auto iter = _nodeTable.begin();
		++iter;
		while(iter!=_nodeTable.end()) {
			assert(iter->subNodeAllAvail());
			assert(iter->subNodeDoesNotExistAtAll());
			++iter;
		}
	}
#endif

	bool IDAllocator::initialize(size_t wholeSize, size_t minSize) {
		size_t layer = 0;
		size_t miniCount = 1;
		while (miniCount< (wholeSize / minSize) ) {
			++layer;
			miniCount = (size_t)1 << layer;
		}
		size_t fullCount = miniCount * 2;
		_minSize = minSize;
		_maxIndex = fullCount - 1;
		_capacity = wholeSize;
		_nodeTable.resize(fullCount);
		// 第[0]个元素永远都不用
		// 第[1]个元素为默认就行
		for (size_t i = 2; i < _nodeTable.size(); ++i) {
			_nodeTable[i].nodeType = (i & 0x1);
		}
		return true;
	}

	bool IDAllocator::allocate(size_t _size, size_t& offset_, uint16_t& id_) {
		struct alloc_t {
			uint16_t level;	// 目前层数 就是 0~15
			uint16_t index;	// 索引就是 0~65535
			alloc_t( uint16_t l, uint16_t i)
				: level(l)
				, index(i) {
			}
			alloc_t()
				: level(0)
				, index(0) {
			}
		};
		_size = _size < _minSize ? _minSize : _size;
		std::vector<alloc_t> alloc_t_list; // 减少 扩容 次数
		alloc_t_list.reserve(32);
		// 我们把第一个元素压入，然后一个一个找合适的大小
		alloc_t_list.emplace_back(
			0, // layer 第0层
			1  // index 第一个元素
		);
		while (!alloc_t_list.empty()) {
			// 获取当前 alloc_t
			alloc_t alloc = alloc_t_list.back();
			alloc_t_list.pop_back();
			//
			auto& node = _nodeTable[alloc.index];
			//
			if (!node.subNodeUnavailableAtAll()) { // 表示其左节点、右节点至少有一个是可以用来分配的
				size_t nodeSize = _capacity >> alloc.level; // 获取当前节点的容量
				size_t halfNodeSize = nodeSize >> 1; // 容量的一半
				if (_size <= halfNodeSize && halfNodeSize >= _minSize ) { // 如果要分配的内存小于或者等于当前的一半，那么把它拆开，用它的子节点来分配
					// 优化PUSH右节点，这样就会优化从左节点去分配
					if( node.subNodeAvailable( node_type_t::right ) ) {
						alloc_t_list.emplace_back( alloc.level + 1, alloc.index*2 +1 ); // 二叉堆特性，右子节点的索引是当前节点索引的 2倍+1
					}
					if( node.subNodeAvailable(node_type_t::left) ) {
						alloc_t_list.emplace_back( alloc.level + 1, alloc.index*2 ); // 二叉堆特性，右子节点的索引是当前节点索引的 2倍+1
					}
				} else {
					// 到了这个分枝，这个节点大小应该是刚刚好了
					// 刚刚好不假，有个问题，大小只是这个节点大小刚刚好，有可能这个节点的一部分已经被分配出去了
					// 所以它只要是有子节点就不能分配
					if(node.subNodeDoesNotExistAtAll()) { // 没有任何子节点，那么，这个节点就可以被分配出去了
						size_t dividedCount = (size_t)1<<alloc.level; 	///> 这一层级被分成了一几块
						size_t& levelStartIndex = dividedCount; ///> 生成一个二叉堆，顶索引为0，从上到下，从左到右顺序累加的值，这里我们取某一Level最左元素对应的索引
						size_t offset = (alloc.index - levelStartIndex) * (_capacity / dividedCount); ///> 计算偏移量
						//
						offset_ = offset;
						id_ = alloc.index;
						_memUsed += nodeSize;
						//
#ifndef NDEBUG
						assert(_allocateRecord.find(alloc.index) == _allocateRecord.end());
						_allocateRecord.insert(alloc.index);
#endif
						updateBinaryHeapNodesForAllocate( node, alloc.level, alloc.index); // 要对这个节点 和 这个节点的父节点更新
						//
						return true;
					}
					else {
						// 分配失败，看剩余的 alloc_t 还有没有能分配出来的
						continue;
					}
				}
			} else {
				// 没有有效的子节点
				continue;
			}
		}
		return false;
	}

	void IDAllocator::updateBinaryHeapNodesForAllocate( IDAllocator::node_t& _node, uint16_t _level, uint16_t _index ) {
		// 首设置它自己为完全不能再分配的状态
		_node.subNodeMarkAsUnavailable();
		auto nodeType = _node.type();
		auto nodeUnavailableAtAll = true;
		// 二叉堆向上回溯，处理两种状态 1. 有没有[左、右]子节点（回溯性强） 2.是不是完全不能再分配使用了
		uint16_t nodeLevel = _level;
		uint16_t nodeIndex = _index;
		//
		node_t* node = &_node;
		// 设置父节点的状态
		while( nodeLevel > 0 ) {
			auto& parentNode = _nodeTable[nodeIndex>>1];
			if(nodeUnavailableAtAll){
				parentNode.subNodeMarkAsUnavailable(nodeType);
				if(!parentNode.subNodeUnavailableAtAll()) {
					nodeUnavailableAtAll = false;
				}
			}
			if(!parentNode.subNodeExist(nodeType)){
				parentNode.subNodeMarkAsExist(nodeType);
			}else if(!nodeUnavailableAtAll) {
				break;
			}
			node = &parentNode;
			nodeType = node->type();
			///
			--nodeLevel;
			nodeIndex = nodeIndex>>1;
			//
			assert(nodeIndex != 0);
		}
	}

	bool IDAllocator::free(uint16_t _id) {
		assert(_id < _nodeTable.size());
		uint16_t layer = 0;
		uint16_t v = _id;
		while (v != 1) {
			layer++;
			v = v >> 1;
		}
		node_t& node = _nodeTable[_id];
		updateBinaryHeapNodesForFree(node, layer, _id);
		//
		_memUsed -= (_capacity >> layer);
		//
#ifndef NDEBUG
		_allocateRecord.erase(_id);
#endif
		//
		return true;
	}

	void IDAllocator::updateBinaryHeapNodesForFree( node_t& _node, uint16_t _level, uint16_t _index ) {
		// 首设置它自己为完全不能再分配的状态
		_node.subNodeMarkAsAllAvailable();
		auto nodeType = _node.type();
		bool subNodeAllAvail = true;
		auto needPassSubNodeAvailState = true;
		// 二叉堆向上回溯，处理两种状态 1. 有没有子节点 2.是不是完全不能再分配使用了（子节点有一个可以使用的，父结点一定可以使用，回溯性强）
		uint16_t nodeLevel = _level;
		uint16_t nodeIndex = _index;
		node_t* node = &_node;
		// 设置父节点的状态
		while( nodeLevel > 0 ) {
			auto& parentNode = _nodeTable[nodeIndex>>1];
			if(subNodeAllAvail) { // 这个节点的数据可以完全回收，那么父节点就可以标记这个节点不存在了（没分配过）
				parentNode.subNodeMarkAsNoneExist(nodeType);
				subNodeAllAvail = parentNode.subNodeDoesNotExistAtAll();
			}
			if(needPassSubNodeAvailState) {
				if(!parentNode.subNodeAvailable(nodeType)){
					parentNode.subNodeMarkAsAvailable(nodeType); // 仅仅是向上传递可分配状态，如果已经是可分配状态了，就不需要再传递了
				} else {
					needPassSubNodeAvailState = false;
				}
			}
			if(!subNodeAllAvail && !needPassSubNodeAvailState) {
				break;
			}
			//
			node = &parentNode;
			nodeType = node->type();
			nodeIndex = nodeIndex>>1;
			nodeLevel =  nodeLevel - 1;
		}
	}

}



