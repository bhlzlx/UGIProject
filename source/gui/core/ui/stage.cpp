#include "stage.h"
#include "core/declare.h"
#include "root.h"
#include "object_factory.h"
#include "g_button.h"
#include "core/ui/ui_content_scaler.h"
#include "core/display_objects/display_object_utility.h"
#include <glm/ext/matrix_transform.hpp>
#include <cmath>

namespace gui {

    void Stage::initialize(float width, float height) {
        ui2dRoot_ = (Root*)ObjectFactory::CreateObject(ObjectType::Root);
        ui2dRoot_->setSize(gui::Size2D<float>(width, height));
    }

    void Stage::onResize(uint32_t width, uint32_t height) {
        screenWidth_ = (float)width;
        screenHeight_ = (float)height;
        UIContentScaler::Instance()->applyChange(screenWidth_, screenHeight_);
        applyContentScaleFactor();
    }

    void Stage::applyContentScaleFactor() {
        if (!ui2dRoot_) return;

        float sf = UIContentScaler::Instance()->scaleFactor;
        // root 的逻辑尺寸 = 屏幕尺寸 / scaleFactor
        // root 的渲染缩放 = scaleFactor
        // 这样所有子节点在设计像素坐标系下工作
        float logicalW = std::ceil(screenWidth_ / sf);
        float logicalH = std::ceil(screenHeight_ / sf);
        ui2dRoot_->setSize({logicalW, logicalH});
        ui2dRoot_->setScale(sf, sf);
    }

    /// 将父空间坐标转换到对象局部空间
    static glm::vec2 pointToLocal(Object* obj, glm::vec2 parentPt) {
        auto dobj = obj->getDisplayObject();
        if (!dobj) return parentPt;
        auto localMat = buildLocalMatrix(dobj.entity());
        glm::mat4 inv = glm::inverse(localMat);
        glm::vec4 p = inv * glm::vec4(parentPt.x, parentPt.y, 0, 1);
        return {p.x, p.y};
    }

    static Object* hitTestRecursive(Object* obj, glm::vec2 parentPt) {
        if (!obj || !obj->visible() || !obj->touchable()) return nullptr;
        auto localPt = pointToLocal(obj, parentPt);
        if (auto* comp = dynamic_cast<Component*>(obj)) {
            // children 在 obj 的局部空间, 传 localPt
            for (int i = comp->numChildren() - 1; i >= 0; --i) {
                auto* child = comp->getChildAt(i);
                if (!child || !child->visible()) continue;
                auto* hit = hitTestRecursive(child, localPt);
                if (hit) return hit;
            }
        }
        if (localPt.x >= 0 && localPt.y >= 0 &&
            localPt.x <= obj->width() && localPt.y <= obj->height())
            return obj;
        return nullptr;
    }

    Object* Stage::hitTest(glm::vec2 screenPos) const {
        if (!ui2dRoot_) {
            return nullptr;
        }
        // 屏幕坐标转 root 局部坐标
        float sf = UIContentScaler::Instance()->scaleFactor;
        glm::vec2 localPt(screenPos.x / sf, screenPos.y / sf);
        return hitTestRecursive(ui2dRoot_, localPt);
    }

    void Stage::onMouseDown(glm::vec2 pos) {
        auto* hit = hitTest(pos);
        pressTarget_ = hit;
        if (hit) hit->bubbleEvent("onTouchBegin");
    }

    void Stage::onMouseUp(glm::vec2 pos) {
        if (pressTarget_) {
            pressTarget_->bubbleEvent("onClick");
            pressTarget_ = nullptr;
        }
    }

    void Stage::onMouseMove(glm::vec2 pos) {
        auto* hit = hitTest(pos);
        if (hit != hoverTarget_) {
            if (hoverTarget_) hoverTarget_->bubbleEvent("onRollOut");
            hoverTarget_ = hit;
            if (hit) hit->bubbleEvent("onRollOver");
        }
    }

    glm::vec2 Stage::RayHitTest(glm::vec3 eye, glm::vec3 dir, glm::vec3 triangle[3]) {
        glm::vec3 e0 = triangle[1] - triangle[0];
        glm::vec3 e1 = triangle[2] - triangle[0];
        glm::vec3 SPVector = eye - triangle[0];
        glm::mat3 DEMatrix(-glm::normalize(dir), e0, e1);
        glm::vec3 possibleSolution = glm::inverse(DEMatrix) * SPVector;
        return possibleSolution;
    }

}
