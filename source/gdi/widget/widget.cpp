#include "widget.h"
#include "component.h"
#include "GdiSystem.h"

namespace ugi {
    namespace gdi {

        uint32_t Widget::depth() const {
            return _depth;
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