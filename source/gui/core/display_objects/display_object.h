#pragma once
#include "../events/event_dispatcher.h"
#include "core/declare.h"
#include "core/display_objects/display_components.h"
#include "entt/src/entt/entity/entity.hpp"
#include "entt/src/entt/entity/fwd.hpp"
#include "glm/fwd.hpp"
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
        operator entt::entity () const {
            return entity();
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
            return entity_ != entt::null;
        }

        DisplayObject parent() const;
        void setParent(DisplayObject parent);
        
        void addChild(DisplayObject child);
        void addChildAt(DisplayObject child, uint32_t index);
        void removeChild(DisplayObject child);
        void removeChildAt(uint32_t index);
        void removeFromParent();


        std::vector<DisplayObject>* children() const;

        void setPosition(glm::vec2 const& val);
        void setSize(glm::vec2 const& val);
        void setPivot(glm::vec2 const& val);
        void setRotation(float val);
        void setSkew(glm::vec2 val);
        void setScale(glm::vec2 val);

        dispcomp::parent_batch& getParentBatch() const;
        dispcomp::basic_transfrom& getBasicTransfrom() const;

        /**
         * @brief Set the Child Index object
         * 其实该改成change child'd index 意思比较准确
         * @param child this object's child
         * @param index target index
         */
        void setChildIndex(DisplayObject child, uint32_t index);
    public:
        static DisplayObject createRootObject();
        static DisplayObject createDisplayObject();
    };

}

