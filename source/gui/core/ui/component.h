#pragma once
#include "core/data_types/ui_types.h"
#include "core/data_types/transition.h"
#include <core/ui_objects/object.h>
#include <vector>
#include "../display_objects/container.h"
#include "core/declare.h"
#include "core/display_objects/display_object.h"
#include "core/events/event_dispatcher.h"
#include "utils/byte_buffer.h"

namespace gui {

    class Component :public Object {
    protected:
        std::vector<Object*>        children_;
        std::vector<Controller*>    controllers_;
        std::vector<Transition>     transitions_;
        // std::vector<Transition>  这个先跳过去
        DisplayObject               root_; // default root
        DisplayObject               container_; // scroll node if need

        bool                        buildingDisplayList_;

        Margin                      margin_;

        bool                        traceBounds_;
        bool                        boundsChanged_;
        ChildrenRenderOrder         childrenRenderOrder_;
        int                         apexIndex_;
        glm::vec2                   alignOffset_;
        glm::vec2                   clipSoftness_;
        int                         sortingChildrenCount_;
        Controller*                 applyingController_;
    public: 
        Component()
            : children_()
            , controllers_()
            , transitions_()
            , margin_()
        {
        }

        virtual void constructFromResource() override;
        virtual void createDisplayObject() override;
        virtual void constructExtension(ByteBuffer& buff) {}
        virtual void constructFromXML() {};

        void constructFromResource(std::vector<Object*> objectPool, uint32_t index);

        void setupOverflow(OverflowType overflow);
        void setupScroll(ByteBuffer& buff);
        void updateClipRect();

        void applyController(Controller* controller);

        void onAddedToStage(EventContext* context);
        void onRemoveFromStage(EventContext* context);

        void buildNativeDisplayList();

        void applyAllControllers();

        void setBoundsChangedFlag();


    };

}