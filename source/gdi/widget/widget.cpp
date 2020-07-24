#include "widget.h"

namespace ugi {
    namespace gdi {

        void Component::_depthSort() {
            // 按Group排序
            std::sort( _widgets.begin(), _widgets.end(), []( Widget* a, Widget* b) {
                uint32_t depth1 = a->_group ? a->_group->_depth : a->_depth;
                uint32_t depth2 = b->_group ? b->_group->_depth : b->_depth;
                return depth1 < depth2;
            });
            // 组内排序
            auto iter = _widgets.begin();
            auto groupBegin = iter;
            while(iter!=_widgets.end()) {
                ++iter;
                if( (*iter)->_group != (*groupBegin)->_group ) {
                    if((*groupBegin)->_group) {
                        std::sort(groupBegin, iter, []( Widget* a, Widget* b) {
                        return a->_depth < b->_depth;
                        });
                    }
                    groupBegin = iter;
                }
            }
            // 最后的组
            if( (*groupBegin)->_group && iter != groupBegin ) {
                std::sort(groupBegin, iter, []( Widget* a, Widget* b) {
                    return a->_depth < b->_depth;
                });
            }
        }

        
        void Component::addWidget( Widget* widget ) {
            if( _widgetsRecord.find(widget) != _widgetsRecord.end()) {
                return;
            }
            _widgetsRecord.insert(widget);
        }

    }
}