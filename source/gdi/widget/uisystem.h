#pragma once

namespace ugi {
    namespace gdi {

        class Component;

        class IComponentDrawingManager {
        protected:
        public:
            virtual void onNeedUpdate( Component* component ) = 0;
            virtual void onAddToDisplayList( Component* component ) = 0;
            virtual void onRemoveFromDisplayList( Component* Component ) = 0;
            virtual void onTick() = 0;
            //
        };

        IComponentDrawingManager* createComponentDrawingManager();

        class UIRoot {
        private:
        public:
            UIRoot() {
            }

            void addComponent( Component* comp );
            //
            void onTick();
        };

        //class ComponentDrawDataManager {
        //private:
        //    // std::map< Component*, GeometryDrawData* >           _drawDatas;
        //    std::set<Component*>                                _updateCollection; ///> 需要更新的列表
        //    std::vector<GeometryDrawData*>                      _drawDatas;
        //public:
        //    ComponentDrawDataManager()
        //        : _drawDatas{}
        //    {}
//
        //    void update();
        //};

    }
}