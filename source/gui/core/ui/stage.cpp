#include "stage.h"
#include "core/declare.h"
#include "root.h"
#include "object_factory.h"
#include <cmath>

namespace gui {

    void Stage::initialize() {
        ui2dRoot_ = (Root*)ObjectFactory::CreateObject(ObjectType::Root);
    }

    void Stage::setScreenSize(uint32_t width, uint32_t height) {
        screenWidth_ = (float)width;
        screenHeight_ = (float)height;
        contentScaler_.applyChange(screenWidth_, screenHeight_);
        applyContentScaleFactor();
    }

    void Stage::applyContentScaleFactor() {
        if (!ui2dRoot_) return;

        float sf = contentScaler_.scaleFactor;
        // root 的逻辑尺寸 = 屏幕尺寸 / scaleFactor
        // root 的渲染缩放 = scaleFactor
        // 这样所有子节点在设计像素坐标系下工作
        float logicalW = std::ceil(screenWidth_ / sf);
        float logicalH = std::ceil(screenHeight_ / sf);
        ui2dRoot_->setSize({logicalW, logicalH});
        ui2dRoot_->setScale(sf, sf);
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
