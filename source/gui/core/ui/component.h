#pragma once
#include <vector>
#include <core/data_types/ui_types.h>
#include <core/data_types/transition.h>
#include <core/ui/object.h>
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
        Controller*                 applyingController_;

        uint32_t                    sortingChildCount_; // 自定义排序优先级的控件数量

        //
        bool                        asBatchNode_;
    public: 
        Component()
            : Object()
            , children_()
            , controllers_()
            , transitions_()
            , buildingDisplayList_(false)
            , margin_()
            , traceBounds_(false)
            , boundsChanged_(false)
            , childrenRenderOrder_(ChildrenRenderOrder::Ascent)
            , sortingChildCount_(0)
            , asBatchNode_(false)
        {
            this->type_ = ObjectType::Component;
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

        void asBatchNode(bool batch);


        Object* addChild(Object* child);
        Object* addChildAt(Object* child, uint32_t index);

        void removeChild(Object* child);
        void removeChildAt(uint32_t index);
private:
        void setChildIndex(Object* child, uint32_t idx);
        uint32_t setChildIndex_(Object* child, uint32_t oldIdx, uint32_t idx);
        uint32_t getInsertPosForSortingOrder(Object* child);

        void syncDisplayList(Object* child);
    };

}