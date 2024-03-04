#pragma once
#include <core/declare.h>

namespace gui {

    enum class CurveType {
        CRSpline,           // 
        Bezier,             // 二次贝塞尔
        CubicBezier,        // 三次贝塞尔
        Straight,           // 没有任何效果的，当直线用
    };

    struct PathPoint {
        glm::vec3   pos;
        glm::vec3   controlPt[2];
        CurveType   curveType;
        //
        PathPoint(glm::vec3 const& pos);
        PathPoint(glm::vec3 const& pos, glm::vec3 const& ctrl);
        PathPoint(glm::vec3 const& pos, glm::vec3 const& ctrl0, glm::vec3 const& ctrl1);
        PathPoint(glm::vec3 const& pos, CurveType type);
    };

    class InterpoPath {
    public:
        struct segment_t {
            CurveType   type;
            float       length;
            int         ptStart;
            int         ptCount;
        };
    private:
        std::vector<segment_t>      segments_;
        std::vector<glm::vec3>      points_;
        float                       fullLength_;
        //
        glm::vec3 onCRSplineCurve(int start, int count, float t) const;
        glm::vec3 onBezierCurve(int start, int count, float t) const;
    public:
        InterpoPath();
        void create(PathPoint const* points, int count);
        void clear();
        glm::vec3 pointAt(float t) const;

        float length() const;
        int segmentCount() const;
        float segmentLength(int segmentIndex) const;
        void queryPoints(int segment, float t0, float t1, std::vector<glm::vec3>& points, std::vector<float>* ts = nullptr, float density = 0.1f) const;
        void queryPoints(std::vector<glm::vec3>& points, float density = 0.1f) const;

    };

}