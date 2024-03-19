#include "stage.h"
#include "core/declare.h"
#include "root.h"
#include "object_factory.h"

namespace gui {

    void Stage::initialize() {
        this->ui2dRoot_ = (Root*)ObjectFactory::CreateObject(ObjectType::Root);
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