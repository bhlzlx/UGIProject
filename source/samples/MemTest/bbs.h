// buddy system allocator 实现
// 仅仅是计算分配位置，这个分配器是跟内存指针无关的
#include <vector>
#include <list>
#include <cstdint>
#include <cstdlib>
#include <set>
#include <unordered_map>
#include <cassert>

namespace bbs {
    
    class IDAllocator {
        enum class node_type_t {
            left = 0,
            right = 1,
        };
        
        struct node_t {

            union {
                struct {
                    uint8_t nodeType            : 1; // 节点类型：左、右？
                    uint8_t nodeBits            : 2; // (nodeBits & 1 << node_type_t::left) 说明有左节点，即左节点上被分配过
                    uint8_t nodeAvailBits       : 2; // (nodeNotAvailableBits & 1 << node_type_t:left)  为真，说明左节点已经完全没有可分配的子节点了
                    uint8_t alignBytes          : 3; // 
                };
                uint8_t k;
            };
            node_t()
                : nodeType(0)               // 默认左节点
                , nodeBits(0x0)             // 默认没有任何子节点
                , nodeAvailBits(0x3)        // 默认节点都可用
                , alignBytes(0)
            {
            }
            node_t( const node_t& _t) {
                k = _t.k;
            }
            node_t& operator==( const node_t& _t ) {
                k = _t.k;
                return *this;
            }
            // 
            bool subNodeExist( node_type_t _nodeMask );         ///> 检查有没有某个节点
            bool subNodeDoesNotExistAtAll();                    ///> 是不是没有任何子节点
            //
            void subNodeMarkAsExist( node_type_t _node );        ///> 标记某子节点存在
            void subNodeMarkAsNoneExist( node_type_t _node );    ///> 标记某子节点不存在
            // ---
            bool subNodeAvailable( node_type_t _nodeMask );     ///> 某个节点是否有空闲空间来继续分配
            // ---
            void subNodeMarkAsUnavailable( node_type_t _node ){ ///> 将某子节点标记为不可利用，如果所有节点都不可用
                nodeAvailBits&= ~(1<<(uint8_t)_node);
            } 
            void subNodeMarkAsUnavailable() {                    ///> 将所有子节点标记为不可分配
                nodeAvailBits = 0;
            }
            void subNodeMarkAsAvailable( node_type_t _node ){     ///> 将某子节点标记为可分配
                nodeAvailBits|= 1<<(uint8_t)_node;
            } 
            void subNodeMarkAsAllAvailable() {
                nodeAvailBits = 3;
            }
            bool subNodeAllAvail() {                            ///> 是不是所有的子节点都可以分配
                return nodeAvailBits == 3;
            }
            // ---
            bool subNodeUnavailableAtAll() {                     ///> 两个子节点都不可用（通常用来调用一下检测一下
                return nodeAvailBits == 0 ; 
            }
            //
            node_type_t type() {
                return (node_type_t)nodeType;
            }
        };
    private:
        std::vector<node_t>     _nodeTable; // m_nodeTable[0] is not used
        size_t                  _capacity;
        size_t                  _minSize;
        size_t                  _maxIndex;
        int64_t                 _memUsed;
#ifndef NDEBUG
        std::set<uint16_t>      _allocateRecord;
#endif
    public:
        IDAllocator() 
            : _capacity(0)
            , _minSize(0)
            , _maxIndex(0)
            , _memUsed(0) {
        }
        bool initialize(size_t _wholeSize, size_t _minSize );
        bool allocate(size_t _size, size_t& offset_, uint16_t& _id );
        bool free( uint16_t _id );
#ifndef NDEBUG
        void empty(); // 这个函数是测试用的，不要用它
#endif
    private:
        // 为分配操作更新二叉堆Node树
        void updateBinaryHeapNodesForAllocate( node_t& _node, uint16_t _level, uint16_t _index );
        // 为回收操作更新二叉堆Node树
        void updateBinaryHeapNodesForFree( node_t& _node, uint16_t _level, uint16_t _index );
    };

    class MemoryAllocator {
    private:
        IDAllocator                                     _idAllocator;
        std::unordered_map<void*, uint16_t>             _idTable;
        void*                                           _memoryChunk;
    public:
        MemoryAllocator() {
        }
        //
        bool initialize( size_t _capacity, size_t _minimumSize ) {
            _memoryChunk = malloc( _capacity );
            if(!_memoryChunk) {
                return false;
            }
            _idAllocator.initialize(_capacity, _minimumSize);
            return true;
        }
        //
        void release() {
            free(_memoryChunk);
        }
        //
        void* allocate( size_t _size ) {
            size_t offset; uint16_t id;
            if(_idAllocator.allocate(_size, offset, id)) {
                void* ptr = (void*)(((uint8_t*)_memoryChunk)+offset);
                _idTable[ptr] = id;
                return ptr;
            }
            return nullptr;
        }
        //
        void free( void* _free ) {
            auto iter = _idTable.find(_free);
            if( iter == _idTable.end() ) {
                assert(false);
                return;
            }
            _idAllocator.free(iter->second);
        }
    };
}