#include "widget.h"
#include "component.h"
#include "GdiSystem.h"

namespace ugi {
    namespace gdi {

        uint32_t Widget::depth() const {
            return _depth;
        }

        void Widget::setDepth(uint32_t depth) {
            _depth = depth;
            // == 
            ComponentLayoutHandler::Action action;
            action.type = ComponentLayoutHandler::Action::Type::Sort;
            action.component = _component;
            _component->system()->trackLayoutAction(action);
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
    }
}