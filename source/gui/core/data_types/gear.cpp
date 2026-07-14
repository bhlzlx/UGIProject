#include "gear.h"
#include <core/ui/object.h>
#include <core/ui/component.h>
#include <core/ui/g_text_field.h>
#include <core/ui/image.h>
#include <core/controller.h>
#include <core/display_objects/display_object.h>

namespace gui {

    void GearBase::setupTween(ByteBuffer& buffer) {
        if (buffer.read<bool>()) {
            _hasTween = true;
            buffer.skip(1 + 4 + 4);
        }
    }

    void GearBase::setup(ByteBuffer& buffer) {
        int ctlIdx = buffer.read<int16_t>();
        if (ctlIdx >= 0 && _owner->parent())
            _controller = _owner->parent()->getControllerAt(ctlIdx);

        int cnt = buffer.read<int16_t>();
        for (int i = 0; i < cnt; ++i) {
            std::string page = buffer.read<csref>();
            if (!page.empty()) addStatus(page, buffer);
        }
        if (buffer.read<bool>()) {
            addStatus("", buffer);
        }
        setupTween(buffer);
    }

    // ============= GearDisplay =============
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
        if (!_controller) return;
        auto cur = _controller->selectedPageId();
        bool show = false;
        for (auto& p : pages) if (p == cur) { show = true; break; }
        if (pages.empty()) show = true;
        _owner->setVisible(show);
    }

    // ============= GearXY =============
    void GearXY::addStatus(std::string const& page, ByteBuffer& buffer) {
        auto& v = page.empty() ? _default : _storage[page];
        v.x  = (float)buffer.read<int>();
        v.y  = (float)buffer.read<int>();
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
    void GearSize::addStatus(std::string const& page, ByteBuffer& buffer) {
        auto& v = page.empty() ? _default : _storage[page];
        v.w = (float)buffer.read<int>();
        v.h = (float)buffer.read<int>();
    }

    void GearSize::apply() {
        if (!_controller) return;
        auto it = _storage.find(_controller->selectedPageId());
        auto& v = (it != _storage.end()) ? it->second : _default;
        _owner->setSize({v.w, v.h});
    }

    void GearSize::updateState() {
        if (!_controller) return;
        auto& v = _storage[_controller->selectedPageId()];
        v.w = _owner->width();
        v.h = _owner->height();
    }

    // ============= GearColor =============
    void GearColor::addStatus(std::string const& page, ByteBuffer& buffer) {
        auto& v = page.empty() ? _default : _storage[page];
        Color4B color = Color4B(buffer.read<uint32_t>());
        color.a = 255;
        Color4B strokeColor = Color4B(buffer.read<uint32_t>());
        strokeColor.a = 255;
        v = {color, strokeColor};
    }

    void GearColor::apply() {
        if (!_controller) return;
        auto it = _storage.find(_controller->selectedPageId());
        auto c = (it != _storage.end()) ? it->second : _default;
        if (auto* img = dynamic_cast<Image*>(_owner)) {
            img->setColor(c.color);
        } else if (auto* txt = dynamic_cast<GTextField*>(_owner)) {
            txt->setColor(c.color);
        }
    }

    // ============= GearLook =============
    void GearLook::addStatus(std::string const& page, ByteBuffer& buffer) {
        auto& v = page.empty() ? _default : _storage[page];
        v.alpha    = buffer.read<float>();
        v.rotation = buffer.read<float>();
        v.grayed   = buffer.read<bool>() ? 1.0f : 0.0f;
    }

    void GearLook::apply() {
        if (!_controller) return;
        auto it = _storage.find(_controller->selectedPageId());
        auto& v = (it != _storage.end()) ? it->second : _default;
        _owner->setAlpha(v.alpha);
        _owner->setRotation(v.rotation);
        _owner->setGrayed(v.grayed > 0.5f);
    }

    // ============= GearText =============
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
