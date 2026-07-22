#include "gear.h"
#include <core/ui/object.h>
#include <core/ui/component.h>
#include <core/ui/g_text_field.h>
#include <core/ui/image.h>
#include <core/controller.h>
#include <core/display_objects/display_object.h>
#include <core/package.h>
#include <core/data_types/tween_manager.h>

namespace gui {

    bool GearBase::disableAllTweenEffect = false;

    void GearBase::setupTween(ByteBuffer& buffer) {
        if (buffer.read<bool>()) {
            _hasTween = true;
            _tweenConfig = new TweenConfig();
            _tweenConfig->tween = true;
            _tweenConfig->easeType = EaseType(buffer.read<uint8_t>());
            _tweenConfig->duration = buffer.read<float>();
            _tweenConfig->repeat = buffer.read<int>();
        }
    }

    void GearBase::setup(ByteBuffer& buffer) {
        int ctlIdx = buffer.read<int16_t>();
        if (ctlIdx >= 0 && _owner->parent()) {
            _controller = _owner->parent()->getControllerAt(ctlIdx);
        }
        init();
        auto pageCount = buffer.read<int16_t>();
        while(true) {
            GearDisplay* display = dynamic_cast<GearDisplay*>(this);
            if(display) {
                buffer.readRefStringArray(display->pages, pageCount);
                break;
            }
            GearDisplay2* display2 = dynamic_cast<GearDisplay2*>(this);
            if(display2) {
                buffer.readRefStringArray(display2->pages, pageCount);
                break;
            }
            // 普通
            for (int i = 0; i < pageCount; ++i) {
                std::string page = buffer.read<csref>();
                if (!page.empty()) {
                    addStatus(page, buffer);
                };
            }
            if (buffer.read<bool>()) {
                addStatus("", buffer);
            }
            break;
        }

        setupTween(buffer);
        //
    }

    // ============= GearDisplay =============
    GearDisplay::GearDisplay(Object* owner) : GearBase(owner) {
        _displayLockToken = 1;
    }

    void GearDisplay::init() {
        pages.clear();
    }

    void GearDisplay::setup(ByteBuffer& buffer) {
        int ctlIdx = buffer.read<int16_t>();
        if (ctlIdx >= 0 && _owner->parent())
            _controller = _owner->parent()->getControllerAt(ctlIdx);
        int cnt = buffer.read<int16_t>();
        pages.resize(cnt);
        for (int i = 0; i < cnt; ++i)
            pages[i] = buffer.read<csref>();
        setupTween(buffer);
    }

    void GearDisplay::apply() {
        _displayLockToken++;
        if (_displayLockToken == 0)
            _displayLockToken = 1;

        if (pages.empty()
            || std::find(pages.begin(), pages.end(), _controller->selectedPageId()) != pages.end())
            _visible = 1;
        else
            _visible = 0;
    }

    uint32_t GearDisplay::addLock() {
        _visible++;
        return _displayLockToken;
    }

    void GearDisplay::releaseLock(uint32_t token) {
        if (token == _displayLockToken)
            _visible--;
    }

    bool GearDisplay::connected() const {
        return _controller == nullptr || _visible > 0;
    }

    // ============= GearDisplay2 =============
    GearDisplay2::GearDisplay2(Object* owner) : GearBase(owner) {
    }

    void GearDisplay2::init() {
        pages.clear();
    }

    void GearDisplay2::setup(ByteBuffer& buffer) {
        int ctlIdx = buffer.read<int16_t>();
        if (ctlIdx >= 0 && _owner->parent())
            _controller = _owner->parent()->getControllerAt(ctlIdx);
        int cnt = buffer.read<int16_t>();
        pages.resize(cnt);
        for (int i = 0; i < cnt; ++i)
            pages[i] = buffer.read<csref>();
        setupTween(buffer);
    }

    void GearDisplay2::apply() {
        if (pages.empty()
            || std::find(pages.begin(), pages.end(), _controller->selectedPageId()) != pages.end())
            _visible = 1;
        else
            _visible = 0;
    }

    bool GearDisplay2::evaluate(bool connected) {
        bool v = _controller == nullptr || _visible > 0;
        if (condition == 0)
            v = v && connected;
        else
            v = v || connected;
        return v;
    }

    // ============= GearXY =============
    void GearXY::addStatus(std::string const& page, ByteBuffer& buffer) {
        auto& v = page.empty() ? _default : _storage[page];
        v.x  = (float)buffer.read<int>();
        v.y  = (float)buffer.read<int>();
    }

    void GearXY::init() {
        if (_owner) {
            auto* parent = _owner->parent();
            float pw = parent ? parent->width() : 1.0f;
            float ph = parent ? parent->height() : 1.0f;
            _default.x = _owner->x();
            _default.y = _owner->y();
            _default.px = _owner->x() / pw;
            _default.py = _owner->y() / ph;
        }
    }

    void GearXY::setup(ByteBuffer& buffer) {
        GearBase::setup(buffer);
        if (buffer.version >= 2 && buffer.read<bool>()) {
            _positionsInPercent = true;
            for (size_t i = 0; i < _storage.size(); ++i) {
                auto pg = buffer.read<csref>();
                auto& v = _storage[pg];
                v.px = buffer.read<float>();
                v.py = buffer.read<float>();
            }
            buffer.read<csref>();
            _default.px = buffer.read<float>();
            _default.py = buffer.read<float>();
        }
    }

    void GearXY::apply() {
        if (!_controller) return;
        auto it = _storage.find(_controller->selectedPageId());
        auto& v = (it != _storage.end()) ? it->second : _default;
        _owner->setPosition({v.x, v.y, 0});
    }

    void GearXY::updateState() {
        if (!_controller) return;
        auto& v = _storage[_controller->selectedPageId()];
        v.x = _owner->x();
        v.y = _owner->y();
    }

    // ============= GearSize =============
    void GearSize::init() {
        if (_owner) {
            _default.w = _owner->width();
            _default.h = _owner->height();
            _default.scaleX = _owner->scaleX();
            _default.scaleY = _owner->scaleY();
        }
    }

    void GearSize::addStatus(std::string const& page, ByteBuffer& buffer) {
        auto& v = page.empty() ? _default : _storage[page];
        v.w = (float)buffer.read<int>();
        v.h = (float)buffer.read<int>();
        v.scaleX = buffer.read<float>();
        v.scaleY = buffer.read<float>();
    }

    void GearSize::apply() {
        if (!_controller) return;
        auto it = _storage.find(_controller->selectedPageId());
        auto& v = (it != _storage.end()) ? it->second : _default;
        _owner->setSize({v.w, v.h});
        _owner->setScale(v.scaleX, v.scaleY);
    }

    void GearSize::updateState() {
        if (!_controller) return;
        auto& v = _storage[_controller->selectedPageId()];
        v.w = _owner->width();
        v.h = _owner->height();
        v.scaleX = _owner->scaleX();
        v.scaleY = _owner->scaleY();
    }

    // ============= GearColor =============
    void GearColor::init() {
        if (_owner) {
            _colorGear = dynamic_cast<IColorGear*>(_owner);
            _outlineColorGear = dynamic_cast<IOutlineColorGear*>(_owner);

            if (_colorGear) {
                _default.color = _colorGear->getColor();
            }
            if (_outlineColorGear) {
                _default.strokeColor = _outlineColorGear->getOutlineColor();
            }
        }
    }

    void GearColor::addStatus(std::string const& page, ByteBuffer& buffer) {
        auto& v = page.empty() ? _default : _storage[page];
        v.color       = buffer.read<uint32_t>();
        v.strokeColor = buffer.read<uint32_t>();
    }

    void GearColor::apply() {
        if (!_controller) return;
        auto it = _storage.find(_controller->selectedPageId());
        auto& gv = (it != _storage.end()) ? it->second : _default;

        if (_tweenConfig && _tweenConfig->tween && Package::constructing_ == 0 && !disableAllTweenEffect) {
            // strokeColor 立即生效（不参与 tween）
            if (_outlineColorGear ) { //&& gv.strokeColor.a > 0
                _owner->lockGear();
                _outlineColorGear->setOutlineColor(gv.strokeColor);
                _owner->unlockGear();
            }

            // 如果已有 tweener 且目标颜色相同，跳过
            if (_tweenConfig->tweener) {
                if (_tweenConfig->tweener->endVal.color4B().val == gv.color) {
                    return;
                }
                _tweenConfig->tweener->kill(true);
                _tweenConfig->tweener = nullptr;
            }

            // 颜色不同则启动新 tween
            if (_colorGear && _colorGear->getColor() != gv.color) {
                if (_owner->checkGearController(int(GearIndex::Display), _controller))
                    _tweenConfig->displayLockToken = _owner->addDisplayLock();

                auto* t = GTween::To(_colorGear->getColor(), gv.color, _tweenConfig->duration)
                    ->setDelay(_tweenConfig->delay)
                    ->setEase(_tweenConfig->easeType)
                    ->setListener(this);
                _tweenConfig->tweener = t;
            }
        } else {
            // 无 tween 时直接设置
            _owner->lockGear();
            if (_colorGear) _colorGear->setColor(gv.color);
            if (_outlineColorGear) //&& gv.strokeColor.a > 0
                _outlineColorGear->setOutlineColor(gv.strokeColor);
            _owner->unlockGear();
        }
    }

    void GearColor::onTweenStart(Tweener* tweener) {
    }

    void GearColor::onTweenUpdate(Tweener* tweener) {
        _owner->lockGear();
        if (_colorGear) _colorGear->setColor(tweener->val.color4B());
        _owner->unlockGear();

        _owner->invalidateBatchingState();
    }

    void GearColor::onTweenComplete(Tweener* tweener) {
        if (_tweenConfig->displayLockToken != 0) {
            _owner->releaseDisplayLock(_tweenConfig->displayLockToken);
            _tweenConfig->displayLockToken = 0;
        }
        _owner->dispatchEvent("onGearStop", this);
    }

    // ============= GearLook =============
    void GearLook::init() {
        if (_owner) {
            _default.alpha = _owner->alpha();
            _default.rotation = _owner->rotation();
            _default.grayed = _owner->grayed();
            _default.touchable = _owner->touchable();
        }
    }

    void GearLook::addStatus(std::string const& page, ByteBuffer& buffer) {
        auto& v = page.empty() ? _default : _storage[page];
        v.alpha    = buffer.read<float>();
        v.rotation = buffer.read<float>();
        v.grayed   = buffer.read<bool>();
        v.touchable= buffer.read<bool>();
    }

    void GearLook::apply() {
        if (!_controller) return;
        auto it = _storage.find(_controller->selectedPageId());
        auto& v = (it != _storage.end()) ? it->second : _default;
        _owner->setAlpha(v.alpha);
        _owner->setRotation(v.rotation);
        _owner->setGrayed(v.grayed);
        _owner->setTouchable(v.touchable);
    }

    // ============= GearText =============
    void GearText::init() {
        if (_owner) {
            if (auto* txt = dynamic_cast<GTextField*>(_owner))
                _default = txt->text();
        }
    }

    void GearText::addStatus(std::string const& page, ByteBuffer& buffer) {
        auto& v = page.empty() ? _default : _storage[page];
        v = buffer.read<csref>();
    }

    void GearText::apply() {
        if (!_controller) return;
        auto it = _storage.find(_controller->selectedPageId());
        auto& v = (it != _storage.end()) ? it->second : _default;
        if (auto* txt = dynamic_cast<GTextField*>(_owner)) txt->setText(v);
    }

    // ============= GearIcon =============
    void GearIcon::init() {
        if (_owner) {
            if (auto* img = dynamic_cast<Image*>(_owner))
                _default = img->getIcon();
        }
    }

    void GearIcon::addStatus(std::string const& page, ByteBuffer& buffer) {
        auto& v = page.empty() ? _default : _storage[page];
        v = buffer.read<csref>();
    }

    void GearIcon::apply() {
        if (!_controller) return;
        auto it = _storage.find(_controller->selectedPageId());
        auto& v = (it != _storage.end()) ? it->second : _default;
        if (auto* img = dynamic_cast<Image*>(_owner)) img->setIcon(v);
    }

    // ============= GearFontSize =============
    void GearFontSize::init() {
        if (_owner) {
            // TODO: 处理 GLabel/ GButton 的情况，其 textField 需要从内部获取
            if (auto* txt = dynamic_cast<GTextField*>(_owner))
                _default = txt->textFormat().fontSize;
        }
    }

    void GearFontSize::addStatus(std::string const& page, ByteBuffer& buffer) {
        auto& v = page.empty() ? _default : _storage[page];
        v = (float)buffer.read<int>();
    }

    void GearFontSize::apply() {
        if (!_controller) return;
        auto it = _storage.find(_controller->selectedPageId());
        float v = (it != _storage.end()) ? it->second : _default;
        if (auto* txt = dynamic_cast<GTextField*>(_owner)) txt->setFontSize(v);
    }

    // ============= Factory =============
    GearBase* createGear(int index, Object* owner) {
        switch (index) {
        case 0: return new GearDisplay(owner);
        case 1: return new GearXY(owner);
        case 2: return new GearSize(owner);
        case 3: return new GearLook(owner);
        case 4: return new GearColor(owner);
        case 5: return nullptr; // GearAnimation 暂未实现
        case 6: return new GearText(owner);
        case 7: return new GearIcon(owner);
        case 8: return new GearDisplay2(owner);
        case 9: return new GearFontSize(owner);
        default: return nullptr;
        }
    }

}
