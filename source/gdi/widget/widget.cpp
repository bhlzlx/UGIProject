#include "widget.h"
#include "component.h"
#include "GdiSystem.h"

namespace ugi {

    namespace gdi {

        NamePool GdiNamePool;

        Widget::Widget( Component* owner, WidgetType type )
            : _owner( owner )
            , _group(nullptr)
            , _rect(0, 0, 16, 16)
            , _depth(0)
            , _type(type)
            , _transformHandle{ 0, 0 }
        {
        }

        uint32_t Widget::depth() const {
            return _depth;
        }

        void Widget::setName(const std::string& name) {
            _name = GdiNamePool.getName(name);
        }

        void Widget::setName(std::string&& name) {
            _name = GdiNamePool.getName(std::move(name));
        }

        void Widget::setKey( const std::string& key ) {
            if(_registComponentKey(key)) {
                _key = GdiNamePool.getName(key);
            }
        }
        void Widget::setKey( std::string&& key ) {
            if(_registComponentKey(key)) {
                _key = GdiNamePool.getName(std::move(key));
            }
        }

        const std::string& Widget::key() {
            return _key.string();
        }

        bool Widget::isStatic() {
            // 低16位是uniform索引，如果是0代表永远不改变，是单位矩阵，不可改
            return ( 0 == (_transformHandle.handle & 0xff));
        }

        bool Widget::_registComponentKey( const std::string& key ) {
            if(_collector) {
                return _owner->registWidget(key, this);
            }
            return false;
        }

        void Widget::setDepth(uint32_t depth) {
            _depth = depth;
            // == 
            ComponentLayoutHandler::Action action;
            action.type = ComponentLayoutHandler::Action::Type::Sort;
            action.component = _owner;
            _owner->system()->trackLayoutAction(action);
        }

        WidgetType Widget::type() const {
            return _type;
        }

        void Widget::setRect(const hgl::RectScope2f& rect) {
            _rect = rect;
        }

        const hgl::RectScope2f& Widget::rect() {
            return _rect;
        }

        void Widget::setTransform( const Transform& transform ) {
            _owner->registTransform( this, transform);          // regist
            _collector->syncTransform(this,transform);              // transform
        }

        void Widget::setColorMask( uint32_t colorMask ) {
            _colorMask = colorMask;
            _collector->syncExtraFlags(this, _colorMask, _extraFlags);
        }

        void Widget::setGray( float gray ) {
            _extraFlags &= 0x00ffffff;
            _extraFlags |= ((uint32_t)(gray*255))<<24;
            _collector->syncExtraFlags(this, _colorMask, _extraFlags);
        }
    }
}