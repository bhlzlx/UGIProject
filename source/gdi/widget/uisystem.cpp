#pragma once
#include "uisystem.h"
#include "component.h"

namespace ugi {
    namespace gdi {

		class ComponentDrawingManager : public IComponentDrawingManager {
		private:
            std::set<Component*>                                _updateCollection; ///> 需要更新的列表
            std::vector<GeometryDrawData*>                      _drawDatas;
        private:

		public:
            ComponentDrawingManager() {
                
            }

            /*  一个 component 更新，那么它的父 component 不需要更新，只是更新它自己就行了
            */
            virtual void onNeedUpdate( Component* component ) override {
            }

            /* 如果一个 component 添加到了父 component 里，那么父 component 则需要重新收集，兄弟component不需要重新收集
            */
            virtual void onAddToDisplayList( Component* component ) override {
            }
            /*如果一个component被移除了，父控件重新不重新收集都无所谓的
            */
            virtual void onRemoveFromDisplayList( Component* Component ) override {

            }

            virtual void onTick() override {

            }
		};

        void UIRoot::addComponent( Component* component, uint32_t depth ) {
            component->setDepth(depth);
            std::sort( _components.begin(), _components.end(), []( Component* a, Component* b)->bool {
                return a->depth() > b->depth();
            });
        }

    }
}