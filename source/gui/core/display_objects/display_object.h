#pragma once
#include "../events/event_dispatcher.h"
#include "core/declare.h"
#include "entt/src/entt/entity/entity.hpp"
#include "entt/src/entt/entity/fwd.hpp"
#include <entt/src/entt/entt.hpp>

namespace gui {

    class DisplayObject : public EventDispatcher {
    private:
        entt::entity        entity_;
    public:
        DisplayObject(entt::entity entity = entt::null)
            : entity_(entity)
        {
        }
        DisplayObject(DisplayObject const& obj)
            : entity_(obj.entity_)
        {
        }
        entt::entity entity() const {
            return entity_;
        }
    public:

        bool operator == (DisplayObject const& obj) const {
            return entity_ == obj.entity_;
        }
        bool operator != (DisplayObject const& obj) const {
            return entity_ != obj.entity_;
        }

        DisplayObject& operator = (DisplayObject const& obj) {
            entity_ = obj.entity_;
            return *this;
        }

        operator bool () const {
            return entity_ == entt::null;
        }

        DisplayObject parent() const;
        
        void addChild(DisplayObject child);
        void addChildAt(DisplayObject child, uint32_t index);
        void removeChild(DisplayObject child);
        void removeChildAt(uint32_t index);
        void removeFromParent();

        void setPosition(Point2D<float> const& pos);
    public:
        static DisplayObject createRootObject();
        static DisplayObject createDisplayObject();
    };

}

