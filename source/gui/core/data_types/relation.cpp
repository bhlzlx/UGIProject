#include "relation.h"
#include <core/declare.h>
#include <core/ui/object.h>
#include <core/ui/component.h>
#include <cmath>
#include <cassert>
#include <functional>

namespace gui {

    // ============= RelationItem =============

    RelationItem::RelationItem(Object* owner)
        : owner_(owner)
        , target_(nullptr)
        , infos_{}
        , targetPos_{0, 0}
        , targetSize_{0, 0}
    {}

    void RelationItem::setTarget(Object* ptr) {
        if (ptr != target_) {
            if (target_) {
                releaseRefTarget(target_);
            }
            target_ = ptr;
            if (ptr) {
                addRefTarget(ptr);
            }
        }
    }

    void RelationItem::addRefTarget(Object* target) {
        if (target != owner_->parent()) {
            // 监听 target 的位置变化
            target->addEventListener(
                UIEvent::PositionChanged,
                std::bind(&RelationItem::onTargetXYChanged, this, std::placeholders::_1),
                EventTag(this)
            );
        }
        // 监听 target 的大小变化
        target->addEventListener(
            UIEvent::SizeChanged,
            std::bind(&RelationItem::onTargetSizeChanged, this, std::placeholders::_1),
            EventTag(this)
        );
        // 记录初始值
        targetPos_.x = target->x();
        targetPos_.y = target->y();
        targetSize_.width = target->width();
        targetSize_.height = target->height();
    }

    void RelationItem::releaseRefTarget(Object* target) {
        target->removeEventListener(UIEvent::PositionChanged, EventTag(this));
        target->removeEventListener(UIEvent::SizeChanged, EventTag(this));
    }

    void RelationItem::add(RelationType type, bool usePercent) {
        if (type == RelationType::Size) {
            add(RelationType::Width, usePercent);
            add(RelationType::Height, usePercent);
            return;
        }
        for (auto& info : infos_) {
            if (info.type == type)
                return;
        }
        internalAdd(type, usePercent);
    }

    void RelationItem::internalAdd(RelationType type, bool usePercent) {
        assert(type != RelationType::Size && "must not be size type!");
        RelationInfo info;
        info.isPercent = usePercent;
        info.type = type;
        info.axis = (type <= RelationType::RightRight
                     || type == RelationType::Width
                     || (type >= RelationType::LeftExtLeft && type <= RelationType::RightExtRight))
                    ? Axis::X : Axis::Y;
        infos_.push_back(info);
        if (usePercent
            || type == RelationType::LeftCenter || type == RelationType::CenterCenter
            || type == RelationType::RightCenter
            || type == RelationType::TopMiddle || type == RelationType::MiddleMiddle
            || type == RelationType::BottomMiddle) {
            owner_->setPixelSnapping(true);
        }
    }

    void RelationItem::remove(RelationType type) {
        if (type == RelationType::Size) {
            remove(RelationType::Width);
            remove(RelationType::Height);
            return;
        }
        for (auto iter = infos_.begin(); iter != infos_.end(); ++iter) {
            if (iter->type == type) {
                infos_.erase(iter);
                break;
            }
        }
    }

    void RelationItem::copyFrom(RelationItem const& src) {
        setTarget(src.target_);
        infos_.clear();
        for (auto& info : src.infos_) {
            infos_.push_back(info);
        }
    }

    void RelationItem::dispose() {
        if (target_) {
            releaseRefTarget(target_);
            target_ = nullptr;
        }
    }

    bool RelationItem::isEmpty() const {
        return infos_.empty();
    }

    // ============= applyOnSelfSizeChanged =============
    // 当 owner 自身大小变化时，调整自身位置
    // 参照 FairyGUI RelationItem.cs:132-182

    void RelationItem::applyOnSelfSizeChanged(float dWidth, float dHeight, bool applyPivot) {
        if (infos_.empty())
            return;

        float ox = owner_->x();
        float oy = owner_->y();

        for (auto& info : infos_) {
            switch (info.type) {
                case RelationType::CenterCenter:
                    owner_->setX(ox - (0.5f - (applyPivot ? owner_->pivot().x : 0)) * dWidth);
                    break;

                case RelationType::RightCenter:
                case RelationType::RightLeft:
                case RelationType::RightRight:
                    owner_->setX(ox - (1.0f - (applyPivot ? owner_->pivot().x : 0)) * dWidth);
                    break;

                case RelationType::MiddleMiddle:
                    owner_->setY(oy - (0.5f - (applyPivot ? owner_->pivot().y : 0)) * dHeight);
                    break;

                case RelationType::BottomMiddle:
                case RelationType::BottomTop:
                case RelationType::BottomBottom:
                    owner_->setY(oy - (1.0f - (applyPivot ? owner_->pivot().y : 0)) * dHeight);
                    break;

                default:
                    break;
            }
        }
    }

    // ============= applyOnXYChanged =============
    // 当 target 位置变化时，调整 owner 的位置
    // 参照 FairyGUI RelationItem.cs:184-261

    void RelationItem::applyOnXYChanged(RelationInfo const& info, float dx, float dy) {
        float tmp;
        switch (info.type) {
            case RelationType::LeftLeft:
            case RelationType::LeftCenter:
            case RelationType::LeftRight:
            case RelationType::CenterCenter:
            case RelationType::RightLeft:
            case RelationType::RightCenter:
            case RelationType::RightRight:
                owner_->setX(owner_->x() + dx);
                break;

            case RelationType::TopTop:
            case RelationType::TopMiddle:
            case RelationType::TopBottom:
            case RelationType::MiddleMiddle:
            case RelationType::BottomTop:
            case RelationType::BottomMiddle:
            case RelationType::BottomBottom:
                owner_->setY(owner_->y() + dy);
                break;

            case RelationType::Width:
            case RelationType::Height:
                break;

            case RelationType::LeftExtLeft:
            case RelationType::LeftExtRight:
                if (owner_ != target_->parent()) {
                    tmp = owner_->xMin();
                    owner_->setWidth(owner_->rawWidth() - dx);
                    owner_->setXMin(tmp + dx);
                } else {
                    owner_->setWidth(owner_->rawWidth() - dx);
                }
                break;

            case RelationType::RightExtLeft:
            case RelationType::RightExtRight:
                if (owner_ != target_->parent()) {
                    tmp = owner_->xMin();
                    owner_->setWidth(owner_->rawWidth() + dx);
                    owner_->setXMin(tmp);
                } else {
                    owner_->setWidth(owner_->rawWidth() + dx);
                }
                break;

            case RelationType::TopExtTop:
            case RelationType::TopExtBottom:
                if (owner_ != target_->parent()) {
                    tmp = owner_->yMin();
                    owner_->setHeight(owner_->rawHeight() - dy);
                    owner_->setYMin(tmp + dy);
                } else {
                    owner_->setHeight(owner_->rawHeight() - dy);
                }
                break;

            case RelationType::BottomExtTop:
            case RelationType::BottomExtBottom:
                if (owner_ != target_->parent()) {
                    tmp = owner_->yMin();
                    owner_->setHeight(owner_->rawHeight() + dy);
                    owner_->setYMin(tmp);
                } else {
                    owner_->setHeight(owner_->rawHeight() + dy);
                }
                break;

            default:
                break;
        }
    }

    // ============= applyOnSizeChanged =============
    // 当 target 大小变化时，调整 owner 的位置和大小
    // 参照 FairyGUI RelationItem.cs:263-561

    void RelationItem::applyOnSizeChanged(RelationInfo const& info) {
        float pos = 0, pivot = 0, delta = 0;
        if (info.axis == Axis::X) {
            if (target_ != owner_->parent()) {
                pos = target_->x();
                if (target_->isPivotAsAnchor())
                    pivot = target_->pivot().x;
            }
            if (info.isPercent) {
                if (targetSize_.width != 0)
                    delta = target_->width() / targetSize_.width;
            } else {
                delta = target_->width() - targetSize_.width;
            }
        } else {
            if (target_ != owner_->parent()) {
                pos = target_->y();
                if (target_->isPivotAsAnchor())
                    pivot = target_->pivot().y;
            }
            if (info.isPercent) {
                if (targetSize_.height != 0)
                    delta = target_->height() / targetSize_.height;
            } else {
                delta = target_->height() - targetSize_.height;
            }
        }

        float v, tmp;

        switch (info.type) {
            // ---- X 轴位置关系 ----
            case RelationType::LeftLeft:
                if (info.isPercent)
                    owner_->setXMin(pos + (owner_->xMin() - pos) * delta);
                else if (pivot != 0)
                    owner_->setX(owner_->x() + delta * (-pivot));
                break;
            case RelationType::LeftCenter:
                if (info.isPercent)
                    owner_->setXMin(pos + (owner_->xMin() - pos) * delta);
                else
                    owner_->setX(owner_->x() + delta * (0.5f - pivot));
                break;
            case RelationType::LeftRight:
                if (info.isPercent)
                    owner_->setXMin(pos + (owner_->xMin() - pos) * delta);
                else
                    owner_->setX(owner_->x() + delta * (1.0f - pivot));
                break;
            case RelationType::CenterCenter:
                if (info.isPercent)
                    owner_->setXMin(pos + (owner_->xMin() + owner_->rawWidth() * 0.5f - pos) * delta
                                    - owner_->rawWidth() * 0.5f);
                else
                    owner_->setX(owner_->x() + delta * (0.5f - pivot));
                break;
            case RelationType::RightLeft:
                if (info.isPercent)
                    owner_->setXMin(pos + (owner_->xMin() + owner_->rawWidth() - pos) * delta
                                    - owner_->rawWidth());
                else if (pivot != 0)
                    owner_->setX(owner_->x() + delta * (-pivot));
                break;
            case RelationType::RightCenter:
                if (info.isPercent)
                    owner_->setXMin(pos + (owner_->xMin() + owner_->rawWidth() - pos) * delta
                                    - owner_->rawWidth());
                else
                    owner_->setX(owner_->x() + delta * (0.5f - pivot));
                break;
            case RelationType::RightRight:
                if (info.isPercent)
                    owner_->setXMin(pos + (owner_->xMin() + owner_->rawWidth() - pos) * delta
                                    - owner_->rawWidth());
                else
                    owner_->setX(owner_->x() + delta * (1.0f - pivot));
                break;

            // ---- Y 轴位置关系 ----
            case RelationType::TopTop:
                if (info.isPercent)
                    owner_->setYMin(pos + (owner_->yMin() - pos) * delta);
                else if (pivot != 0)
                    owner_->setY(owner_->y() + delta * (-pivot));
                break;
            case RelationType::TopMiddle:
                if (info.isPercent)
                    owner_->setYMin(pos + (owner_->yMin() - pos) * delta);
                else
                    owner_->setY(owner_->y() + delta * (0.5f - pivot));
                break;
            case RelationType::TopBottom:
                if (info.isPercent)
                    owner_->setYMin(pos + (owner_->yMin() - pos) * delta);
                else
                    owner_->setY(owner_->y() + delta * (1.0f - pivot));
                break;
            case RelationType::MiddleMiddle:
                if (info.isPercent)
                    owner_->setYMin(pos + (owner_->yMin() + owner_->rawHeight() * 0.5f - pos) * delta
                                    - owner_->rawHeight() * 0.5f);
                else
                    owner_->setY(owner_->y() + delta * (0.5f - pivot));
                break;
            case RelationType::BottomTop:
                if (info.isPercent)
                    owner_->setYMin(pos + (owner_->yMin() + owner_->rawHeight() - pos) * delta
                                    - owner_->rawHeight());
                else if (pivot != 0)
                    owner_->setY(owner_->y() + delta * (-pivot));
                break;
            case RelationType::BottomMiddle:
                if (info.isPercent)
                    owner_->setYMin(pos + (owner_->yMin() + owner_->rawHeight() - pos) * delta
                                    - owner_->rawHeight());
                else
                    owner_->setY(owner_->y() + delta * (0.5f - pivot));
                break;
            case RelationType::BottomBottom:
                if (info.isPercent)
                    owner_->setYMin(pos + (owner_->yMin() + owner_->rawHeight() - pos) * delta
                                    - owner_->rawHeight());
                else
                    owner_->setY(owner_->y() + delta * (1.0f - pivot));
                break;

            // ---- 大小关系 ----
            case RelationType::Width:
                if (owner_->underConstruct_ && owner_ == target_->parent())
                    v = owner_->sourceWidth() - target_->initWidth();
                else
                    v = owner_->rawWidth() - targetSize_.width;
                if (info.isPercent)
                    v = v * delta;
                if (target_ == owner_->parent()) {
                    if (owner_->isPivotAsAnchor()) {
                        tmp = owner_->xMin();
                        owner_->setSize({target_->width() + v, owner_->rawHeight()});
                        owner_->setXMin(tmp);
                    } else {
                        owner_->setSize({target_->width() + v, owner_->rawHeight()});
                    }
                } else {
                    owner_->setWidth(target_->width() + v);
                }
                break;
            case RelationType::Height:
                if (owner_->underConstruct_ && owner_ == target_->parent())
                    v = owner_->sourceHeight() - target_->initHeight();
                else
                    v = owner_->rawHeight() - targetSize_.height;
                if (info.isPercent)
                    v = v * delta;
                if (target_ == owner_->parent()) {
                    if (owner_->isPivotAsAnchor()) {
                        tmp = owner_->yMin();
                        owner_->setSize({owner_->rawWidth(), target_->height() + v});
                        owner_->setYMin(tmp);
                    } else {
                        owner_->setSize({owner_->rawWidth(), target_->height() + v});
                    }
                } else {
                    owner_->setHeight(target_->height() + v);
                }
                break;

            // ---- 扩展关系 (拉伸) ----
            case RelationType::LeftExtLeft:
                tmp = owner_->xMin();
                if (info.isPercent)
                    v = pos + (tmp - pos) * delta - tmp;
                else
                    v = delta * (-pivot);
                owner_->setWidth(owner_->rawWidth() - v);
                owner_->setXMin(tmp + v);
                break;
            case RelationType::LeftExtRight:
                tmp = owner_->xMin();
                if (info.isPercent)
                    v = pos + (tmp - pos) * delta - tmp;
                else
                    v = delta * (1.0f - pivot);
                owner_->setWidth(owner_->rawWidth() - v);
                owner_->setXMin(tmp + v);
                break;
            case RelationType::RightExtLeft:
                tmp = owner_->xMin();
                if (info.isPercent)
                    v = pos + (tmp + owner_->rawWidth() - pos) * delta - (tmp + owner_->rawWidth());
                else
                    v = delta * (-pivot);
                owner_->setWidth(owner_->rawWidth() + v);
                owner_->setXMin(tmp);
                break;
            case RelationType::RightExtRight:
                tmp = owner_->xMin();
                if (info.isPercent) {
                    if (owner_ == target_->parent()) {
                        if (owner_->underConstruct_)
                            owner_->setWidth(pos + target_->width() - target_->width() * pivot
                                + (owner_->sourceWidth() - pos - target_->initWidth()
                                   + target_->initWidth() * pivot) * delta);
                        else
                            owner_->setWidth(pos + (owner_->rawWidth() - pos) * delta);
                    } else {
                        v = pos + (tmp + owner_->rawWidth() - pos) * delta
                            - (tmp + owner_->rawWidth());
                        owner_->setWidth(owner_->rawWidth() + v);
                        owner_->setXMin(tmp);
                    }
                } else {
                    if (owner_ == target_->parent()) {
                        if (owner_->underConstruct_)
                            owner_->setWidth(owner_->sourceWidth()
                                + (target_->width() - target_->initWidth()) * (1.0f - pivot));
                        else
                            owner_->setWidth(owner_->rawWidth() + delta * (1.0f - pivot));
                    } else {
                        v = delta * (1.0f - pivot);
                        owner_->setWidth(owner_->rawWidth() + v);
                        owner_->setXMin(tmp);
                    }
                }
                break;

            case RelationType::TopExtTop:
                tmp = owner_->yMin();
                if (info.isPercent)
                    v = pos + (tmp - pos) * delta - tmp;
                else
                    v = delta * (-pivot);
                owner_->setHeight(owner_->rawHeight() - v);
                owner_->setYMin(tmp + v);
                break;
            case RelationType::TopExtBottom:
                tmp = owner_->yMin();
                if (info.isPercent)
                    v = pos + (tmp - pos) * delta - tmp;
                else
                    v = delta * (1.0f - pivot);
                owner_->setHeight(owner_->rawHeight() - v);
                owner_->setYMin(tmp + v);
                break;
            case RelationType::BottomExtTop:
                tmp = owner_->yMin();
                if (info.isPercent)
                    v = pos + (tmp + owner_->rawHeight() - pos) * delta
                        - (tmp + owner_->rawHeight());
                else
                    v = delta * (-pivot);
                owner_->setHeight(owner_->rawHeight() + v);
                owner_->setYMin(tmp);
                break;
            case RelationType::BottomExtBottom:
                tmp = owner_->yMin();
                if (info.isPercent) {
                    if (owner_ == target_->parent()) {
                        if (owner_->underConstruct_)
                            owner_->setHeight(pos + target_->height() - target_->height() * pivot
                                + (owner_->sourceHeight() - pos - target_->initHeight()
                                   + target_->initHeight() * pivot) * delta);
                        else
                            owner_->setHeight(pos + (owner_->rawHeight() - pos) * delta);
                    } else {
                        v = pos + (tmp + owner_->rawHeight() - pos) * delta
                            - (tmp + owner_->rawHeight());
                        owner_->setHeight(owner_->rawHeight() + v);
                        owner_->setYMin(tmp);
                    }
                } else {
                    if (owner_ == target_->parent()) {
                        if (owner_->underConstruct_)
                            owner_->setHeight(owner_->sourceHeight()
                                + (target_->height() - target_->initHeight()) * (1.0f - pivot));
                        else
                            owner_->setHeight(owner_->rawHeight() + delta * (1.0f - pivot));
                    } else {
                        v = delta * (1.0f - pivot);
                        owner_->setHeight(owner_->rawHeight() + v);
                        owner_->setYMin(tmp);
                    }
                }
                break;

            default:
                break;
        }
    }

    // ============= Event Callbacks =============

    void RelationItem::onTargetXYChanged(EventContext* context) {
        if (owner_->relations_.handling != nullptr)
        {
            targetPos_.x = target_->x();
            targetPos_.y = target_->y();
            return;
        }

        owner_->relations_.handling = target_;

        float ox = owner_->x();
        float oy = owner_->y();
        float dx = target_->x() - targetPos_.x;
        float dy = target_->y() - targetPos_.y;

        for (auto& info : infos_)
            applyOnXYChanged(info, dx, dy);

        targetPos_.x = target_->x();
        targetPos_.y = target_->y();

        owner_->relations_.handling = nullptr;
    }

    void RelationItem::onTargetSizeChanged(EventContext* context) {
        if (owner_->relations_.handling != nullptr)
        {
            targetSize_.width = target_->width();
            targetSize_.height = target_->height();
            return;
        }

        owner_->relations_.handling = target_;

        float ox = owner_->x();
        float oy = owner_->y();
        float ow = owner_->rawWidth();
        float oh = owner_->rawHeight();

        for (auto& info : infos_)
            applyOnSizeChanged(info);

        targetSize_.width = target_->width();
        targetSize_.height = target_->height();

        owner_->relations_.handling = nullptr;
    }

    // ============= Relations =============

    Relations::Relations(Object* owner)
        : owner_(owner)
        , items_{}
        , handling(nullptr)
    {}

    Relations::~Relations() {
        clearAll();
    }

    void Relations::add(Object* target, RelationType type, bool usePercent) {
        for (auto item : items_) {
            if (item->getTarget() == target) {
                item->add(type, usePercent);
                return;
            }
        }
        RelationItem* newItem = new RelationItem(owner_);
        newItem->setTarget(target);
        newItem->add(type, usePercent);
        items_.push_back(newItem);
    }

    void Relations::remove(Object* target, RelationType type) {
        int cnt = (int)items_.size();
        int i = 0;
        while (i < cnt) {
            RelationItem* item = items_[i];
            if (item->getTarget() == target) {
                item->remove(type);
                if (item->isEmpty()) {
                    item->dispose();
                    delete item;
                    items_.erase(items_.begin() + i);
                    cnt--;
                    continue;
                }
            }
            i++;
        }
    }

    bool Relations::contains(Object* target) const {
        for (auto item : items_) {
            if (item->getTarget() == target)
                return true;
        }
        return false;
    }

    void Relations::clearFor(Object* target) {
        int cnt = (int)items_.size();
        int i = 0;
        while (i < cnt) {
            RelationItem* item = items_[i];
            if (item->getTarget() == target) {
                item->dispose();
                delete item;
                items_.erase(items_.begin() + i);
                cnt--;
            } else {
                i++;
            }
        }
        handling = nullptr;
    }

    void Relations::clearAll() {
        for (auto item : items_) {
            item->dispose();
            delete item;
        }
        items_.clear();
        handling = nullptr;
    }

    void Relations::copyFrom(Relations const& other) {
        clearAll();
        for (auto ri : other.items_) {
            RelationItem* item = new RelationItem(owner_);
            item->copyFrom(*ri);
            items_.push_back(item);
        }
    }

    void Relations::onOwnerSizeChanged(float dWidth, float dHeight, bool applyPivot) {
        if (items_.empty())
            return;
        for (auto item : items_)
            item->applyOnSelfSizeChanged(dWidth, dHeight, applyPivot);
    }

    bool Relations::isEmpty() const {
        return items_.empty();
    }

    void Relations::setup(ByteBuffer& buffer, bool parentToChild) {
        int cnt = buffer.read<uint8_t>();
        Object* target = nullptr;
        for (int i = 0; i < cnt; i++) {
            int targetIndex = buffer.read<int16_t>();
            if (targetIndex == -1)
                target = owner_->parent();
            else if (parentToChild)
                target = ((Component*)owner_)->getChildAt(targetIndex);
            else
                target = owner_->parent()->getChildAt(targetIndex);

            RelationItem* newItem = new RelationItem(owner_);
            newItem->setTarget(target);
            items_.push_back(newItem);

            int cnt2 = buffer.read<uint8_t>();
            for (int j = 0; j < cnt2; j++) {
                RelationType rt = (RelationType)buffer.read<uint8_t>();
                bool usePercent = buffer.read<bool>();
                newItem->internalAdd(rt, usePercent);
            }
        }
    }

}
