#include "interpolatable_path.h"
#include <algorithm>

namespace gui {

    inline float clampf(float value, float min_inclusive, float max_inclusive)
    {
        if (min_inclusive > max_inclusive) {
            std::swap(min_inclusive, max_inclusive);
        }
        return value < min_inclusive ? min_inclusive : value < max_inclusive? value : max_inclusive;
    }

    class SplineHelper {
    private:
        std::vector<glm::vec3>      points_;
    public:
        SplineHelper() {}
        ~SplineHelper() {}
        //
        std::vector<glm::vec3> const& points() const {
            return points_;
        }
        void addPoint(glm::vec3 const& point) {
            points_.push_back(point);
        }
        bool empty() const {
            return points_.empty();
        }
        size_t size() const {
            return points_.size();
        }
        InterpoPath::segment_t createSegmnet(std::vector<glm::vec3>& output) {
            int cnt = points_.size();
            auto first = points_.front();
            auto last = points_.back();
            points_.reserve(points_.size() + 3);
            points_.insert(points_.begin(), first);
            points_.push_back(last);
            points_.push_back(last);
            cnt += 3;
            //
            InterpoPath::segment_t seg;
            seg.type = CurveType::CRSpline;
            seg.ptStart = output.size();
            seg.ptCount = cnt;
            seg.length = 0;
            for(int i = 1; i<cnt; ++i) {
                seg.length += (points_[i] - points_[i-1]).length();
            }
            for(auto const& pt: points_) {
                points_.push_back(pt);
            }
            points_.clear();
            return seg;
        }
    };

    PathPoint::PathPoint(glm::vec3 const& p)
        : pos(p)
        , controlPt {p}
        , curveType(CurveType::Straight)
    {}

    PathPoint::PathPoint(glm::vec3 const& p, glm::vec3 const& ctrl)
        : pos(p)
        , controlPt{p}
        , curveType(CurveType::Bezier)
    {}

    PathPoint::PathPoint(glm::vec3 const& p, glm::vec3 const& ctrl0, glm::vec3 const& ctrl1)
        : pos(p)
        , controlPt {ctrl0, ctrl1}
        , curveType(CurveType::CubicBezier)
    {}

    PathPoint::PathPoint(glm::vec3 const& p, CurveType type)
        : pos(p)
        , controlPt {}
        , curveType(type)
    {}

    // copy from fgui
    static float repeat(float t, float length) {
        return t - floor(t / length) * length;
    }
    glm::vec3 InterpoPath::onCRSplineCurve(int start, int count, float t) const {
        int adjustedIndex = floor(t * (count - 4)) + start; //Since the equation works with 4 points, we adjust the starting point depending on t to return a point on the specific segment
        glm::vec3 result;
        glm::vec3 p0 = points_[adjustedIndex];
        glm::vec3 p1 = points_[adjustedIndex + 1];
        glm::vec3 p2 = points_[adjustedIndex + 2];
        glm::vec3 p3 = points_[adjustedIndex + 3];
        float adjustedT = (t == 1.f) ? 1.f : repeat(t * (count - 4), 1.f); // Then we adjust t to be that value on that new piece of segment... for t == 1f don't use repeat (that would return 0f);

        float t0 = ((-adjustedT + 2.f) * adjustedT - 1.f) * adjustedT * 0.5f;
        float t1 = (((3.f * adjustedT - 5.f) * adjustedT) * adjustedT + 2.f) * 0.5f;
        float t2 = ((-3.f * adjustedT + 4.f) * adjustedT + 1.f) * adjustedT * 0.5f;
        float t3 = ((adjustedT - 1.f) * adjustedT * adjustedT) * 0.5f;

        result.x = p0.x * t0 + p1.x * t1 + p2.x * t2 + p3.x * t3;
        result.y = p0.y * t0 + p1.y * t1 + p2.y * t2 + p3.y * t3;
        result.z = p0.z * t0 + p1.z * t1 + p2.z * t2 + p3.z * t3;

        return result;
    }

    glm::vec3 InterpoPath::onBezierCurve(int start, int count, float t) const {
        float t2 = 1.0f - t;
        glm::vec3 p0 = points_[start];
        glm::vec3 p1 = points_[start + 1];
        glm::vec3 cp0 = points_[start + 2];
        if (count == 4) {
            glm::vec3 cp1 = points_[start + 3];
            return t2 * t2 * t2 * p0 + 3.f * t2 * t2 * t * cp0 + 3.f * t2 * t * t * cp1 + t * t * t * p1;
        } else {
            return t2 * t2 * p0 + 2.f * t2 * t * cp0 + t * t * p1;
        }
    }

    InterpoPath::InterpoPath()
        : segments_()
        , points_()
        , fullLength_(0)
    {}

    void InterpoPath::create(PathPoint const* points, int count) {
        segments_.clear();
        points_.clear();
        SplineHelper splineHelper;
        fullLength_ = 0.0f;
        if(0 == count) {
            return;
        }
        PathPoint const* prev = points;
        if(CurveType::CRSpline == prev->curveType) {
            splineHelper.addPoint(prev->pos);
        }
        for(int i = 1; i<count; i++) {
            PathPoint const* cur = points + 1;
            if(prev->curveType != CurveType::CRSpline) {
                segment_t seg;
                seg.type = prev->curveType;
                seg.ptStart = points_.size();
                if (prev->curveType == CurveType::Straight)
                {
                    seg.ptCount = 2;
                    points_.push_back(prev->pos);
                    points_.push_back(cur->pos);
                }
                else if (prev->curveType == CurveType::Bezier)
                {
                    seg.ptCount = 3;
                    points_.push_back(prev->pos);
                    points_.push_back(cur->pos);
                    points_.push_back(prev->controlPt[0]);
                }
                else if (prev->curveType == CurveType::CubicBezier)
                {
                    seg.ptCount = 4;
                    points_.push_back(prev->pos);
                    points_.push_back(cur->pos);
                    points_.push_back(prev->controlPt[0]);
                    points_.push_back(prev->controlPt[1]);
                }
                seg.length = (prev->pos - cur->pos).length();
                fullLength_ += seg.length;
                segments_.push_back(seg);
            }
            if(cur->curveType != CurveType::CRSpline) {
                if(!splineHelper.empty()) {
                    splineHelper.addPoint(cur->pos);
                    auto seg = splineHelper.createSegmnet(points_);
                    segments_.push_back(seg);
                    fullLength_ += seg.length;
                }
            } else {
                splineHelper.addPoint(cur->pos);
            }
            prev = cur;
        }
        if(splineHelper.size() > 1) {
            auto seg = splineHelper.createSegmnet(points_);
            segments_.push_back(seg);
            fullLength_ += seg.length;
        }
    }

    void InterpoPath::clear() {
        segments_.clear();
        points_.clear();
    }

    glm::vec3 InterpoPath::pointAt(float t) const {
        t = clampf(t, 0, 1);
        int cnt = segments_.size();
        if(0 == cnt) {
            return glm::vec3();
        }
        segment_t seg;
        if(t == 1.f) {
            seg = segments_[cnt-1];
            if (seg.type == CurveType::Straight) {
                return glm::mix(points_[seg.ptStart], points_[seg.ptStart + 1], t);
            }
            else if (seg.type == CurveType::Bezier || seg.type == CurveType::CubicBezier) {
                return onBezierCurve(seg.ptStart, seg.ptCount, t);
            } else {
                return onCRSplineCurve(seg.ptStart, seg.ptCount, t);
            }
        }

        float len = t * fullLength_;
        glm::vec3 pt;
        for (int i = 0; i < cnt; i++)
        {
            seg = segments_[i];

            len -= seg.length;
            if (len < 0) {
                t = 1 + len / seg.length;
                if (seg.type == CurveType::Straight)
                    pt = glm::mix(points_[seg.ptStart], points_[seg.ptStart + 1], t);
                else if (seg.type == CurveType::Bezier || seg.type == CurveType::CubicBezier) {
                    pt = onBezierCurve(seg.ptStart, seg.ptCount, t);
                } else {
                    pt = onCRSplineCurve(seg.ptStart, seg.ptCount, t);
                }
                break;
            }
        }
        return pt;
    }

    float InterpoPath::length() const {
        return fullLength_;
    }

    int InterpoPath::segmentCount() const {
        return segments_.size();
    }

    float InterpoPath::segmentLength(int segmentIndex) const {
        return segments_[segmentIndex].length;
    }

    void InterpoPath::queryPoints(int segment, float t0, float t1, std::vector<glm::vec3>& points, std::vector<float>* ts , float density ) const {
        if (ts != nullptr) {
            ts->push_back(t0);
        }
        segment_t seg = segments_[segment];
        if (seg.type == CurveType::Straight) {
            points.push_back(glm::mix(points_[seg.ptStart], points_[seg.ptStart + 1], t0));
            points.push_back(glm::mix(points_[seg.ptStart], points_[seg.ptStart + 1], t1));
        }
        else if (seg.type == CurveType::Bezier || seg.type == CurveType::CubicBezier)
        {
            points.push_back(onBezierCurve(seg.ptStart, seg.ptCount, t0));
            int SmoothAmount = (int)std::min<float>(seg.length * density, 50);
            for (int j = 0; j <= SmoothAmount; j++)
            {
                float t = (float)j / SmoothAmount;
                if (t > t0 && t < t1)
                {
                    points.push_back(onBezierCurve(seg.ptStart, seg.ptCount, t));
                    if (ts != nullptr)
                        ts->push_back(t);
                }
            }
            points.push_back(onBezierCurve(seg.ptStart, seg.ptCount, t1));
        }
        else
        {
            points.push_back(onCRSplineCurve(seg.ptStart, seg.ptCount, t0));
            int SmoothAmount = (int)std::min<float>(seg.length * density, 50);
            for (int j = 0; j <= SmoothAmount; j++)
            {
                float t = (float)j / SmoothAmount;
                if (t > t0 && t < t1)
                {
                    points.push_back(onCRSplineCurve(seg.ptStart, seg.ptCount, t));
                    if (ts != nullptr)
                        ts->push_back(t);
                }
            }
            points.push_back(onCRSplineCurve(seg.ptStart, seg.ptCount, t1));
        }

        if (ts != nullptr)
            ts->push_back(t1);
    }

    void InterpoPath::queryPoints(std::vector<glm::vec3>& points, float density) const {
        int cnt = (int)segments_.size();
        for (int i = 0; i < cnt; i++) {
            queryPoints(i, 0, 1, points, nullptr, density);
        }
    }


}