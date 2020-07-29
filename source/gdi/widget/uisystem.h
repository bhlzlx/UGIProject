#pragma once
#include <vector>

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
            std::vector<Component*>     _components;
            IComponentDrawingManager*   _drawingManager;
		private:
			void onAddComponent( Component* component ) {
                _drawingManager->onAddToDisplayList(component);
            }
			void onUpdateComponent( Component* component ) {
                _drawingManager->onNeedUpdate(component);
            }
        public:
            UIRoot() {
            }

            void addComponent( Component* component, uint32_t depth = 0 );
            //
            void onTick();
            //

        };

    }
}